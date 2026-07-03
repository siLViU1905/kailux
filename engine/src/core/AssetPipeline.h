#pragma once

#include "Scene.h"
#include "mesh/MeshRegistry.h"
#include "physics/PhysicsRegistry.h"
#include "texture/TextureRegistry.h"
#include "FrameData.h"
#include "utilities/Queue.h"

namespace kailux
{
    class AssetPipeline
    {
    public:
        struct PendingMeshData
        {
            std::string path;
            MeshLoader::LoadData data;
            std::string name;
            MeshTransformData transform;
            MeshMaterialData material;
            MeshType type{MeshType::Unknown};
            PhysicsBodyType bodyType{PhysicsBodyType::Static};
        };

        AssetPipeline(Context &context,
                      MeshRegistry &meshRegistry,
                      TextureRegistry &textureRegistry,
                      TransferManager &transferManager,
                      Scene &scene,
                      std::span<FrameData> frames);

        void poll();

        Queue<PendingMeshData> &getPendingQueue();

        bool isCached(std::string_view path) const;

        struct MeshCache
        {
            MeshHandle meshHandle;
            TextureSetHandle materialHandle;
            uint32_t count = 1;
        };

        std::optional<MeshCache> uncache(std::string_view path);

        using OnLog = std::move_only_function<void(std::string_view)>;
        void setOnInfoLog(OnLog &&callback);

    private:
        void processBuiltinMesh(const PendingMeshData &data);

        void processLoadedMesh(const PendingMeshData &data);

        entt::entity createParentMeshEntity(const PendingMeshData &data);

        std::vector<TextureSetHandle> loadAndRegisterMaterials(
            std::span<const TextureRegistry::MaterialData> materials);

        MeshHandle uploadMeshDataToRegistry(const MeshRegistry::MeshData &data);

        TextureSetHandle uploadMaterialDataToRegistry(const TextureRegistry::MaterialData &data);

        void cacheMesh(std::string_view path, MeshHandle meshHandle, TextureSetHandle materialHandle);

        std::array<DescriptorSetUpdateInfo, TextureRegistry::s_TextureTypes.size()>
        makeDescriptorSetUpdateInfo(TextureSetHandle slotToOverwrite, const TextureSet &replacementSet);

        std::reference_wrapper<Context>         m_Context;
        std::reference_wrapper<MeshRegistry>    m_MeshRegistry;
        std::reference_wrapper<TextureRegistry> m_TextureRegistry;
        std::reference_wrapper<TransferManager> m_TransferManager;
        std::reference_wrapper<Scene>           m_Scene;
        std::span<FrameData>                    m_Frames;

        Queue<PendingMeshData>                     m_PendingMeshData;
        std::unordered_map<std::string, MeshCache> m_MeshCache;

        OnLog m_OnInfoLog;
    };
}
