#include "AssetPipeline.h"

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
        : m_Context(context),
          m_MeshRegistry(meshRegistry),
          m_TextureRegistry(textureRegistry),
          m_TransferManager(transferManager),
          m_Scene(scene),
          m_Frames(frames)
    {
    }

    Queue<AssetPipeline::PendingMeshData> &AssetPipeline::getPendingQueue()
    {
        return m_PendingMeshData;
    }

    void AssetPipeline::setOnInfoLog(OnLog &&callback)
    {
        m_OnInfoLog = std::move(callback);
    }

    bool AssetPipeline::isCached(std::string_view path) const
    {
        return m_MeshCache.contains(std::string(path));
    }

    void AssetPipeline::cacheMesh(std::string_view path, MeshHandle meshHandle, TextureSetHandle materialHandle)
    {
        auto strPath = std::string(path);
        if (isCached(path))
        {
            ++m_MeshCache[strPath].count;
            return;
        }
        m_MeshCache[strPath] = {meshHandle, materialHandle};
    }

    std::optional<AssetPipeline::MeshCache> AssetPipeline::uncache(std::string_view path)
    {
        auto it = m_MeshCache.find(std::string(path));
        if (it == m_MeshCache.end())
            return std::nullopt;
        auto &count = it->second.count;
        if (count > 1)
        {
            --count;
            return std::nullopt;
        }
        std::optional cache = it->second;
        m_MeshCache.erase(it);
        return cache;
    }

    void AssetPipeline::poll()
    {
        if (auto data = m_PendingMeshData.tryPop())
        {
            if (data->type == MeshType::Unknown)
                return;
            if (data->type != MeshType::Loaded)
                processBuiltinMesh(*data);
            else
                processLoadedMesh(*data);
        }
    }

    std::array<DescriptorSetUpdateInfo, TextureRegistry::kTextureTypes.size()>
    AssetPipeline::makeDescriptorSetUpdateInfo(TextureSetHandle slotToOverwrite, const TextureSet &replacementSet)
    {
        auto makeUpdateInfo = [slotToOverwrite](uint32_t binding, const auto &texture) -> DescriptorSetUpdateInfo
        {
            return {
                binding,
                slotToOverwrite.index,
                DescriptorSetImageInfo(
                    texture->getSampler(),
                    texture->getImageView(),
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    1
                )
            };
        };
        uint32_t textureIndex = 0;
        std::array updateInfos = {
            makeUpdateInfo(MainPass::kMeshTextureBindStart + textureIndex++, replacementSet.albedo),
            makeUpdateInfo(MainPass::kMeshTextureBindStart + textureIndex++, replacementSet.normal),
            makeUpdateInfo(MainPass::kMeshTextureBindStart + textureIndex++, replacementSet.roughness),
            makeUpdateInfo(MainPass::kMeshTextureBindStart + textureIndex++, replacementSet.metallic),
            makeUpdateInfo(MainPass::kMeshTextureBindStart + textureIndex++, replacementSet.ao)
        };
        static_assert(TextureRegistry::kTextureTypes.size() == updateInfos.size(),
                      "Texture type count mismatch");
        return updateInfos;
    }

    entt::entity AssetPipeline::createParentMeshEntity(const PendingMeshData &data)
    {
        Scene &scene = m_Scene;

        auto rootName = data.name.empty() ? scene.getMeshEntityName() : data.name;
        auto parentEntity = scene.createParentEntity(rootName);

        auto &entityReg = scene.getEntityRegistry();
        entityReg.emplace<HierarchyComponent>(parentEntity);

        auto &parentTransform = entityReg.emplace<TransformComponent>(parentEntity);
        parentTransform.transform.position = data.transform.position;
        parentTransform.transform.rotation = data.transform.rotation;
        parentTransform.transform.scale = data.transform.scale;

        entityReg.emplace<MeshMaterialData>(parentEntity, data.material);

        return parentEntity;
    }

    std::vector<TextureSetHandle> AssetPipeline::loadAndRegisterMaterials(
        std::span<const TextureRegistry::MaterialData> materials)
    {
        std::vector<TextureSetHandle> handles;
        handles.reserve(materials.size());
        for (const auto &material: materials)
            handles.push_back(uploadMaterialDataToRegistry(material));

        return handles;
    }

    MeshHandle AssetPipeline::uploadMeshDataToRegistry(const MeshRegistry::MeshData &data)
    {
        std::vector<Buffer> stagingBuffers;
        auto otc = OneTimeCommand::create(m_Context);
        auto handle = m_MeshRegistry.get().upload(m_Context, otc.getCommandBuffer(), data, stagingBuffers);
        otc.submit(m_Context);
        return handle;
    }

    TextureSetHandle AssetPipeline::uploadMaterialDataToRegistry(const TextureRegistry::MaterialData &data)
    {
        auto &textureRegistry = m_TextureRegistry.get();

        auto result = textureRegistry.createSetFromMaterialData(m_Context, data);
        auto handle = textureRegistry.registerTextureSet(result.set);

        auto updateInfos = makeDescriptorSetUpdateInfo(handle, result.set);

        for (const auto &frame: m_Frames)
            frame.getDescriptorSet().updateInfo(m_Context, updateInfos);

        if (!result.uploads.empty())
            m_TransferManager.get().enqueueImages(
                m_Context,
                std::move(result.uploads),
                std::move(result.staging),
                [this, handle]()
                {
                    const auto &set = m_TextureRegistry.get().view(handle);
                    auto infos = makeDescriptorSetUpdateInfo(handle, set);
                    for (const auto &frame: m_Frames)
                        frame.getDescriptorSet().updateInfo(m_Context, infos);
                }
            );

        return handle;
    }

    void AssetPipeline::processBuiltinMesh(const PendingMeshData &data)
    {
        Scene &scene = m_Scene;
        auto &textureRegistry = m_TextureRegistry.get();
        auto &meshRegistry = m_MeshRegistry.get();

        auto now = Clock::now();
        std::string meshName;
        auto createMeshEntity = [&](auto meshHandle, const auto &vertices)
        {
            const auto &material = textureRegistry.view(textureRegistry.getDefaultSetHandle());
            auto textureHandle = textureRegistry.registerTextureSet(material);

            meshName = data.name.empty() ? scene.getMeshEntityName() : data.name;
            scene.createMeshEntity(
                meshName,
                {
                    meshHandle,
                    data.path,
                    data.type,
                    Geometry::computeBoundingSphere(vertices)
                },
                textureHandle,
                data.transform,
                data.material
            );
        };
        switch (data.type)
        {
            case MeshType::Cube:
                createMeshEntity(
                    meshRegistry.getBuiltins().cube,
                    MeshRegistry::generate_cube().vertices
                );
                break;
            case MeshType::Sphere:
                createMeshEntity(
                    meshRegistry.getBuiltins().sphere,
                    MeshRegistry::generate_sphere().vertices
                );
                break;
            default:
                break;
        }
        m_OnInfoLog(std::format("Loaded '{}' successfully in {:.3f}ms.",
                                meshName, Clock::get_elapsed<float, TimeType::Milliseconds>(now)));
    }

    void AssetPipeline::processLoadedMesh(const PendingMeshData &data)
    {
        Scene &scene = m_Scene;

        auto now = Clock::now();
        const auto &loadData = data.data;

        auto parentEntity = createParentMeshEntity(data);

        auto firstSubmeshKey = std::format("{}_sub0", data.path);
        bool modelIsCached = isCached(firstSubmeshKey);

        std::vector<TextureSetHandle> loadedMaterialHandles;
        if (!modelIsCached)
            loadedMaterialHandles = loadAndRegisterMaterials(loadData.materials);

        auto pendingEntities = create_shared<std::vector<entt::entity> >();

        m_TransferManager.get().enqueueBuffer(
            m_Context,
            [this, &data, &loadData, &loadedMaterialHandles, parentEntity, modelIsCached, pendingEntities]
    (auto cmd) -> TransferManager::RecordResult
            {
                Scene &scene = m_Scene;
                auto &meshRegistry = m_MeshRegistry.get();
                auto &textureRegistry = m_TextureRegistry.get();

                TransferManager::RecordResult result;
                uint32_t submeshIndex = 0;

                for (const auto &submesh: loadData.submeshes)
                {
                    auto cacheKey = std::format("{}_sub{}", data.path, submeshIndex);

                    MeshHandle meshHandle;
                    TextureSetHandle textureHandle;

                    if (isCached(cacheKey))
                    {
                        auto cache = m_MeshCache.at(cacheKey);
                        const auto &material = textureRegistry.view(cache.materialHandle);
                        textureHandle = textureRegistry.registerTextureSet(material);
                        auto updateInfos = makeDescriptorSetUpdateInfo(textureHandle, material);
                        for (const auto &frame: m_Frames)
                            frame.getDescriptorSet().updateInfo(m_Context, updateInfos);
                        meshHandle = cache.meshHandle;
                    } else
                    {
                        meshHandle = meshRegistry.upload(m_Context, cmd, submesh.meshData, result.staging);
                        textureHandle = loadedMaterialHandles[submesh.materialIndex];

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

                    cacheMesh(cacheKey, meshHandle, textureHandle);

                    auto rootName = data.name.empty() ? scene.getMeshEntityName() : data.name;
                    auto submeshName = std::format("{}_{}", rootName,
                                                   submesh.name.empty() ? std::to_string(submeshIndex) : submesh.name);

                    auto childEntity = scene.createMeshEntity(
                        submeshName,
                        {
                            meshHandle,
                            data.path,
                            data.type,
                            submesh.boundingSphere
                        },
                        textureHandle,
                        {},
                        data.material,
                        parentEntity
                    );

                    auto &childTransform = scene.getEntityRegistry().get<TransformComponent>(childEntity);
                    childTransform.submeshLocalMatrix = submesh.localTransform;

                    scene.getEntityRegistry().emplace<PendingUploadComponent>(childEntity);
                    pendingEntities->push_back(childEntity);

                    ++submeshIndex;
                }
                return result;
            },
            [this, pendingEntities]()
            {
                auto &registry = m_Scene.get().getEntityRegistry();
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
        m_OnInfoLog(std::format("Loaded '{}' successfully with {} submeshes and {} unique materials in {}ms.",
                                name,
                                loadData.submeshes.size(),
                                loadData.materials.size(),
                                Clock::get_elapsed<float, TimeType::Milliseconds>(now)
        ));
    }
}
