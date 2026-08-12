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

    Queue<AssetPipeline::PendingMeshData> &AssetPipeline::getPendingQueue()
    {
        return mPendingMeshData;
    }

    void AssetPipeline::setOnInfoLog(OnLog &&callback)
    {
        mOnInfoLog = std::move(callback);
    }

    void AssetPipeline::setOnWarningLog(OnLog &&callback)
    {
        mOnWarningLog = std::move(callback);
    }

    void AssetPipeline::setOnAttachPhysics(OnAttachPhysics &&callback)
    {
        mOnAttachPhysics = std::move(callback);
    }

    bool AssetPipeline::isCached(std::string_view path) const
    {
        return mMeshCache.contains(std::string(path));
    }

    void AssetPipeline::cacheMesh(std::string_view path, MeshHandle meshHandle, MaterialHandle materialHandle)
    {
        auto strPath = std::string(path);
        if (isCached(path))
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
                texture.getSampler(),
                texture.getImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1
            )
        };
    }

    std::optional<AssetPipeline::MeshCache> AssetPipeline::uncache(std::string_view path)
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

    void AssetPipeline::poll()
    {
        if (auto data = mPendingMeshData.tryPop())
        {
            if (data->type == MeshType::Unknown)
                return;
            if (data->type != MeshType::Loaded)
                processBuiltinMesh(*data);
            else
                processLoadedMesh(*data);
        }
    }

    entt::entity AssetPipeline::createParentMeshEntity(const PendingMeshData &data)
    {
        Scene &scene = mScene;
        auto  &entityReg = scene.getEntityRegistry();

        const MeshSourceComponent source{data.path, data.type};

        if (data.target != entt::null)
        {
            scene.attachMeshSource(data.target, source);
            entityReg.emplace_or_replace<MeshMaterialData>(data.target, data.material);
            return data.target;
        }

        const auto rootName     = data.name.empty() ? scene.getMeshEntityName() : data.name;
        const auto parentEntity = scene.createParentEntity(rootName);

        scene.setLocalTransform(parentEntity, data.transform);
        scene.attachMeshSource(parentEntity, source);
        entityReg.emplace_or_replace<MeshMaterialData>(parentEntity, data.material);

        return parentEntity;

    }

    std::vector<MaterialHandle> AssetPipeline::loadAndRegisterMaterials(
        std::span<const TextureRegistry::MaterialData> materials)
    {
        std::vector<MaterialHandle> handles;
        handles.reserve(materials.size());
        for (const auto &material: materials)
            handles.push_back(uploadMaterialDataToRegistry(material));

        return handles;
    }

    MaterialHandle AssetPipeline::uploadMaterialDataToRegistry(const TextureRegistry::MaterialData &data)
    {
        auto &textureRegistry = mTextureRegistry.get();
        auto result = textureRegistry.createMaterialFromData(mContext, data);

        std::vector<DescriptorSetUpdateInfo> writes;
        writes.reserve(result.handles.size());
        for (auto handle : result.handles)
            writes.push_back(make_texture_write(handle, textureRegistry.getTexture(handle)));

        for (const auto &frame: mFrames)
            frame.getMeshDescriptorSet().updateInfo(mContext, writes);

        if (!result.uploads.empty())
            mTransferManager.get().enqueueImages(
                mContext,
                std::move(result.uploads),
                std::move(result.staging),
                []() {}
            );

        return textureRegistry.registerMaterial(result.slot).value_or(textureRegistry.getDefaultMaterialHandle());
    }

    void AssetPipeline::processBuiltinMesh(const PendingMeshData &data)
    {
        Scene &scene = mScene;
        auto &textureRegistry = mTextureRegistry.get();
        auto &meshRegistry = mMeshRegistry.get();

        auto now = Clock::now();
        std::string meshName;
        auto createMeshEntity = [&](auto meshHandle, const auto &vertices)
        {
            const auto materialHandle = textureRegistry.getDefaultMaterialHandle();
            const MeshComponent component{
                meshHandle,
                Geometry::computeBoundingSphere(vertices)
            };
            const MeshSourceComponent source{data.path, data.type};

            entt::entity entity{entt::null};

            if (data.target != entt::null)
            {
                entity   = data.target;
                meshName = scene.getEntityRegistry().get<TagComponent>(entity).name;

                if (!scene.attachMesh(entity, component, materialHandle, data.material))
                {
                    mOnWarningLog("The maximum number of meshes has been reached");
                    return;
                }
                scene.attachMeshSource(entity, source);
            }
            else
            {
                meshName = data.name.empty() ? scene.getMeshEntityName() : data.name;
                const auto created = scene.createMeshEntity(meshName, component, materialHandle, data.transform, data.material);
                if (!created)
                {
                    mOnWarningLog("The maximum number of meshes has been reached");
                    return;
                }
                scene.attachMeshSource(*created, source);
                entity = *created;
            }

            if (data.physics)
                mOnAttachPhysics(entity, *data.physics);
        };
        switch (data.type)
        {
            case MeshType::Cube:
                createMeshEntity(
                    meshRegistry.getBuiltins().cube,
                    MeshGeometry::generate_cube().vertices
                );
                break;
            case MeshType::Sphere:
                createMeshEntity(
                    meshRegistry.getBuiltins().sphere,
                    MeshGeometry::generate_sphere().vertices
                );
                break;
            default:
                break;
        }
        mOnInfoLog(std::format("Loaded '{}' successfully in {:.3f}ms.",
                                meshName, Clock::get_elapsed<float, TimeType::Milliseconds>(now)));
    }

    void AssetPipeline::processLoadedMesh(const PendingMeshData &data)
    {
        Scene& scene = mScene;
        auto remainingMeshes = details::kMaxMeshes - static_cast<uint32_t>(scene.getEntityRegistry().view<MeshComponent>().size());
        if (data.data.submeshes.size() > remainingMeshes)
        {
            mOnWarningLog("The maximum number of meshes will be reached, mesh not loaded");
            return;
        }
        auto now = Clock::now();
        const auto &loadData = data.data;

        auto parentEntity = createParentMeshEntity(data);

        auto firstSubmeshKey = std::format("{}_sub0", data.path);
        bool modelIsCached = isCached(firstSubmeshKey);

        std::vector<MaterialHandle> loadedMaterialHandles;
        if (!modelIsCached)
            loadedMaterialHandles = loadAndRegisterMaterials(loadData.materials);

        auto pendingEntities = create_shared<std::vector<entt::entity> >();
        mTransferManager.get().enqueueBuffer(
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

                    if (isCached(cacheKey))
                    {
                        auto cache = mMeshCache.at(cacheKey);
                        materialHandle = cache.materialHandle;
                        meshHandle = cache.meshHandle;
                    } else
                    {
                        meshHandle = meshRegistry.upload(mContext, cmd, submesh.meshData, result.staging);
                        materialHandle = loadedMaterialHandles[submesh.materialIndex];

                        auto regions = meshRegistry.getRegions(meshHandle);
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

                    cacheMesh(cacheKey, meshHandle, materialHandle);

                    const auto &rootName = mScene.get().getEntityRegistry().get<TagComponent>(parentEntity).name;
                    auto submeshName = std::format("{}_{}", rootName,
                                                   submesh.name.empty() ? std::to_string(submeshIndex) : submesh.name);

                    if (auto childEntity = scene.createMeshEntity(
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
                        auto &childTransform = scene.getEntityRegistry().get<TransformComponent>(*childEntity);
                        glm::vec3 t, s, skew;
                        glm::quat r;
                        glm::vec4 persp;
                        glm::decompose(submesh.localTransform, s, r, t, skew, persp);
                        childTransform.transform.position = t;
                        childTransform.transform.rotation = r;
                        childTransform.transform.scale = s;

                        scene.getEntityRegistry().emplace<PendingUploadComponent>(*childEntity);
                        pendingEntities->push_back(*childEntity);

                        ++submeshIndex;
                    }
                    else
                        mOnWarningLog("The maximum number of meshes has been reached");
                }
                return result;
            },
            [this, pendingEntities]()
            {
                auto &registry = mScene.get().getEntityRegistry();
                for (auto entity: *pendingEntities)
                    if (registry.valid(entity))
                        registry.remove<PendingUploadComponent>(entity);
            }
        );

        auto &entityReg = scene.getEntityRegistry();
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
