#pragma once

#include "scene/Scene.h"
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
            entt::entity target{entt::null};
            std::string path;
            MeshLoader::LoadData data;
            std::string name;
            MeshTransformData transform;
            MeshMaterialData material;
            MeshType type{MeshType::Unknown};
            std::optional<PhysicsRecord> physics{std::nullopt};
        };

        AssetPipeline(Context &context,
                      MeshRegistry &meshRegistry,
                      TextureRegistry &textureRegistry,
                      TransferManager &transferManager,
                      Scene &scene,
                      std::span<FrameData> frames);

        void Poll();

        Queue<PendingMeshData> &GetPendingQueue();

        bool IsCached(std::string_view path) const;

        struct MeshCache
        {
            MeshHandle     meshHandle;
            MaterialHandle materialHandle;
            uint32_t       count{1};
        };

        std::optional<MeshCache> Uncache(std::string_view path);

        using OnLog = std::move_only_function<void(std::string_view)>;
        void SetOnInfoLog(OnLog &&callback);
        void SetOnWarningLog(OnLog &&callback);

        using OnAttachPhysics = std::move_only_function<void(entt::entity, PhysicsRecord)>;
        void SetOnAttachPhysics(OnAttachPhysics &&callback);

    private:
        void ProcessBuiltinMesh(const PendingMeshData &data);

        void ProcessLoadedMesh(const PendingMeshData &data);

        entt::entity CreateParentMeshEntity(const PendingMeshData &data);

        std::vector<MaterialHandle> LoadAndRegisterMaterials(
            std::span<const TextureRegistry::MaterialData> materials);

        MaterialHandle UploadMaterialDataToRegistry(const TextureRegistry::MaterialData &data);

        void CacheMesh(std::string_view path, MeshHandle meshHandle, MaterialHandle materialHandle);

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
