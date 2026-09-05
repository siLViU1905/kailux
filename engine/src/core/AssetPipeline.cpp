#include "AssetPipeline.h"

#include <glm/gtx/matrix_decompose.hpp>

#include "Clock.h"
#include "Geometry.h"
#include "components/entt/CachedPhysicsData.h"
#include "components/entt/HierarchyComponent.h"
#include "components/entt/PendingUploadComponent.h"
#include "components/entt/TagComponent.h"
#include "components/gpu/TransformComponent.h"

namespace kailux
{
    AssetPipeline::AssetPipeline(Context &context,
                                 MeshRegistry &meshRegistry,
                                 TextureRegistry &textureRegistry,
                                 TransferManager &transferManager,
                                 Scene &scene,
                                 std::span<FrameData> frames)
        : mContext(context),
          mMeshRegistry(meshRegistry),
          mTextureRegistry(textureRegistry),
          mTransferManager(transferManager),
          mScene(scene),
          mFrames(frames)
    {
    }

    Queue<AssetPipeline::PendingMeshData> &AssetPipeline::GetPendingQueue()
    {
        return mPendingMeshData;
    }

    void AssetPipeline::SetOnInfoLog(OnLog &&callback)
    {
        mOnInfoLog = std::move(callback);
    }

    void AssetPipeline::SetOnWarningLog(OnLog &&callback)
    {
        mOnWarningLog = std::move(callback);
    }

    void AssetPipeline::SetOnAttachPhysics(OnAttachPhysics &&callback)
    {
        mOnAttachPhysics = std::move(callback);
    }

    bool AssetPipeline::IsCached(std::string_view path) const
    {
        return mMeshCache.contains(std::string(path));
    }

    void AssetPipeline::CacheMesh(std::string_view path, MeshHandle meshHandle, MaterialHandle materialHandle)
    {
        auto strPath = std::string(path);
        if (IsCached(path))
        {
            ++mMeshCache[strPath].count;
            return;
        }
        mMeshCache[strPath] = {meshHandle, materialHandle};
    }

    DescriptorSetUpdateInfo AssetPipeline::make_texture_write(TextureHandle handle, const Texture &texture)
    {
        return {
            MainPass::kMeshTextureBindStart,
            handle.index,
            DescriptorSetImageInfo(
                texture.GetSampler(),
                texture.GetImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1
            )
        };
    }

    std::optional<AssetPipeline::MeshCache> AssetPipeline::Uncache(std::string_view path)
    {
        auto it = mMeshCache.find(std::string(path));
        if (it == mMeshCache.end())
            return std::nullopt;
        auto &count = it->second.count;
        if (count > 1)
        {
            --count;
            return std::nullopt;
        }
        std::optional cache = it->second;
        mMeshCache.erase(it);
        return cache;
    }

    void AssetPipeline::Poll()
    {
        if (auto data = mPendingMeshData.TryPop())
        {
            if (data->type == MeshType::Unknown)
                return;
            if (data->type != MeshType::Loaded)
                ProcessBuiltinMesh(*data);
            else
                ProcessLoadedMesh(*data);
        }
    }

    entt::entity AssetPipeline::CreateParentMeshEntity(const PendingMeshData &data)
    {
        Scene &scene = mScene;
        auto  &entityReg = scene.GetEntityRegistry();

        const MeshSourceComponent source{data.path, data.type};

        if (data.target != entt::null)
        {
            scene.AttachMeshSource(data.target, source);
            entityReg.emplace_or_replace<MeshMaterialData>(data.target, data.material);
            return data.target;
        }

        const auto rootName     = data.name.empty() ? scene.GetMeshEntityName() : data.name;
        const auto parentEntity = scene.CreateParentEntity(rootName);

        scene.SetLocalTransform(parentEntity, data.transform);
        scene.AttachMeshSource(parentEntity, source);
        entityReg.emplace_or_replace<MeshMaterialData>(parentEntity, data.material);

        return parentEntity;

    }

    std::vector<MaterialHandle> AssetPipeline::LoadAndRegisterMaterials(
        std::span<const TextureRegistry::MaterialData> materials)
    {
        std::vector<MaterialHandle> handles;
        handles.reserve(materials.size());
        for (const auto &material: materials)
            handles.push_back(UploadMaterialDataToRegistry(material));

        return handles;
    }

    MaterialHandle AssetPipeline::UploadMaterialDataToRegistry(const TextureRegistry::MaterialData &data)
    {
        auto &textureRegistry = mTextureRegistry.get();
        auto result = textureRegistry.CreateMaterialFromData(mContext, data);

        std::vector<DescriptorSetUpdateInfo> writes;
        writes.reserve(result.handles.size());
        for (auto handle : result.handles)
            writes.push_back(make_texture_write(handle, textureRegistry.GetTexture(handle)));

        for (const auto &frame: mFrames)
            frame.GetMeshDescriptorSet().UpdateInfo(mContext, writes);

        if (!result.uploads.empty())
            mTransferManager.get().EnqueueImages(
                mContext,
                std::move(result.uploads),
                std::move(result.staging),
                []() {}
            );

        return textureRegistry.RegisterMaterial(result.slot).value_or(textureRegistry.GetDefaultMaterialHandle());
    }

    void AssetPipeline::ProcessBuiltinMesh(const PendingMeshData &data)
    {
        Scene &scene = mScene;
        auto &textureRegistry = mTextureRegistry.get();
        auto &meshRegistry = mMeshRegistry.get();

        auto now = Clock::now();
        std::string meshName;
        auto createMeshEntity = [&](auto meshHandle, const auto &vertices)
        {
            const auto materialHandle = textureRegistry.GetDefaultMaterialHandle();
            const MeshComponent component{
                meshHandle,
                Geometry::compute_bounding_sphere(vertices)
            };
            const MeshSourceComponent source{data.path, data.type};

            entt::entity entity{entt::null};

            if (data.target != entt::null)
            {
                entity   = data.target;
                meshName = scene.GetEntityRegistry().get<TagComponent>(entity).name;

                if (!scene.AttachMesh(entity, component, materialHandle, data.material))
                {
                    mOnWarningLog("The maximum number of meshes has been reached");
                    return;
                }
                scene.AttachMeshSource(entity, source);
            }
            else
            {
                meshName = data.name.empty() ? scene.GetMeshEntityName() : data.name;
                const auto created = scene.CreateMeshEntity(meshName, component, materialHandle, data.transform, data.material);
                if (!created)
                {
                    mOnWarningLog(created.error());
                    return;
                }
                scene.AttachMeshSource(*created, source);
                entity = *created;
            }

            if (data.physics)
                mOnAttachPhysics(entity, *data.physics);
        };
        switch (data.type)
        {
            case MeshType::Cube:
                createMeshEntity(
                    meshRegistry.GetBuiltins().cube,
                    MeshGeometry::generate_cube().vertices
                );
                break;
            case MeshType::Sphere:
                createMeshEntity(
                    meshRegistry.GetBuiltins().sphere,
                    MeshGeometry::generate_sphere().vertices
                );
                break;
            default:
                break;
        }
        mOnInfoLog(std::format("Loaded '{}' successfully in {:.3f}ms.",
                                meshName, Clock::get_elapsed<float, TimeType::Milliseconds>(now)));
    }

    void AssetPipeline::ProcessLoadedMesh(const PendingMeshData &data)
    {
        Scene& scene = mScene;
        auto remainingMeshes = details::kMaxMeshes - static_cast<uint32_t>(scene.GetEntityRegistry().view<MeshComponent>().size());
        if (data.data.submeshes.size() > remainingMeshes)
        {
            mOnWarningLog("The maximum number of meshes will be reached, mesh not loaded");
            return;
        }
        auto now = Clock::now();
        const auto &loadData = data.data;

        auto parentEntity = CreateParentMeshEntity(data);

        auto firstSubmeshKey = std::format("{}_sub0", data.path);
        bool modelIsCached = IsCached(firstSubmeshKey);

        std::vector<MaterialHandle> loadedMaterialHandles;
        if (!modelIsCached)
            loadedMaterialHandles = LoadAndRegisterMaterials(loadData.materials);

        auto pendingEntities = create_shared<std::vector<entt::entity> >();
        mTransferManager.get().EnqueueBuffer(
            mContext,
            [&]
    (auto cmd) -> TransferManager::RecordResult
            {
                auto &meshRegistry = mMeshRegistry.get();
                auto &textureRegistry = mTextureRegistry.get();

                TransferManager::RecordResult result;
                uint32_t submeshIndex = 0;
                for (const auto &submesh: loadData.submeshes)
                {
                    auto cacheKey = std::format("{}_sub{}", data.path, submeshIndex);

                    MeshHandle meshHandle;
                    MaterialHandle materialHandle;

                    if (IsCached(cacheKey))
                    {
                        auto cache = mMeshCache.at(cacheKey);
                        materialHandle = cache.materialHandle;
                        meshHandle = cache.meshHandle;
                    } else
                    {
                        meshHandle = meshRegistry.Upload(mContext, cmd, submesh.meshData, result.staging);
                        materialHandle = loadedMaterialHandles[submesh.materialIndex];

                        auto regions = meshRegistry.GetRegions(meshHandle);
                        result.resources.emplace_back(
                            regions.vertexBuffer, regions.vertexOffset, regions.vertexSize,
                            vk::PipelineStageFlagBits2::eVertexInput,
                            vk::AccessFlagBits2::eVertexAttributeRead
                        );
                        result.resources.emplace_back(
                            regions.indexBuffer, regions.indexOffset, regions.indexSize,
                            vk::PipelineStageFlagBits2::eVertexInput,
                            vk::AccessFlagBits2::eIndexRead
                        );
                    }
                    if (data.physics)
                        mOnAttachPhysics(parentEntity, *data.physics);

                    CacheMesh(cacheKey, meshHandle, materialHandle);

                    const auto &rootName = mScene.get().GetEntityRegistry().get<TagComponent>(parentEntity).name;
                    auto submeshName = std::format("{}_{}", rootName,
                                                   submesh.name.empty() ? std::to_string(submeshIndex) : submesh.name);

                    if (auto childEntity = scene.CreateMeshEntity(
                        submeshName,
                        {
                            meshHandle,
                            submesh.boundingSphere
                        },
                        materialHandle,
                        {},
                        data.material,
                        parentEntity
                    ))
                    {
                        auto &childTransform = scene.GetEntityRegistry().get<TransformComponent>(*childEntity);
                        glm::vec3 t, s, skew;
                        glm::quat r;
                        glm::vec4 persp;
                        glm::decompose(submesh.localTransform, s, r, t, skew, persp);
                        childTransform.transform.position = t;
                        childTransform.transform.rotation = r;
                        childTransform.transform.scale = s;

                        scene.GetEntityRegistry().emplace<PendingUploadComponent>(*childEntity);
                        pendingEntities->push_back(*childEntity);

                        ++submeshIndex;
                    }
                    else
                        mOnWarningLog(childEntity.error());
                }
                return result;
            },
            [this, pendingEntities]()
            {
                auto &registry = mScene.get().GetEntityRegistry();
                for (auto entity: *pendingEntities)
                    if (registry.valid(entity))
                        registry.remove<PendingUploadComponent>(entity);
            }
        );

        auto &entityReg = scene.GetEntityRegistry();
        auto &physicsCache = entityReg.emplace<CachedPhysicsData>(parentEntity);
        physicsCache.meshType = data.type;
        physicsCache.submeshes.reserve(loadData.submeshes.size());
        for (const auto &submesh: loadData.submeshes)
            physicsCache.submeshes.emplace_back(
                std::move(submesh.meshData.vertices),
                std::move(submesh.meshData.indices),
                submesh.localTransform
            );

        const auto &name = entityReg.get<TagComponent>(parentEntity).name;
        mOnInfoLog(std::format("Loaded '{}' successfully with {} submeshes and {} unique materials in {}ms.",
                                name,
                                loadData.submeshes.size(),
                                loadData.materials.size(),
                                Clock::get_elapsed<float, TimeType::Milliseconds>(now)
        ));
    }
}
