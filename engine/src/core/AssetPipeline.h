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
            PhysicsBodyType bodyType{PhysicsBodyType::Unknown};
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
            MeshHandle     meshHandle;
            MaterialHandle materialHandle;
            uint32_t       count{1};
        };

        std::optional<MeshCache> uncache(std::string_view path);

        using OnLog = std::move_only_function<void(std::string_view)>;
        void setOnInfoLog(OnLog &&callback);
        void setOnWarningLog(OnLog &&callback);

        using OnAttachPhysics = std::move_only_function<void(entt::entity, PhysicsBodyType)>;
        void setOnAttachPhysics(OnAttachPhysics &&callback);

    private:
        void processBuiltinMesh(const PendingMeshData &data);

        void processLoadedMesh(const PendingMeshData &data);

        entt::entity createParentMeshEntity(const PendingMeshData &data);

        std::vector<MaterialHandle> loadAndRegisterMaterials(
            std::span<const TextureRegistry::MaterialData> materials);

        MaterialHandle uploadMaterialDataToRegistry(const TextureRegistry::MaterialData &data);

        void cacheMesh(std::string_view path, MeshHandle meshHandle, TextureSetHandle materialHandle);

        static DescriptorSetUpdateInfo make_texture_write(TextureHandle handle, const Texture& texture);

        std::reference_wrapper<Context>         mContext;
        std::reference_wrapper<MeshRegistry>    mMeshRegistry;
        std::reference_wrapper<TextureRegistry> mTextureRegistry;
        std::reference_wrapper<TransferManager> mTransferManager;
        std::reference_wrapper<Scene>           mScene;
        std::span<FrameData>                    mFrames;

        Queue<PendingMeshData>                     mPendingMeshData;
        std::unordered_map<std::string, MeshCache> mMeshCache;

        OnLog mOnInfoLog;
        OnLog mOnWarningLog;

        OnAttachPhysics mOnAttachPhysics;
    };
}
