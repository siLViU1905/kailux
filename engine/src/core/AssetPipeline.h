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
            entt::entity                 target{entt::null};
            std::string                  path;
            MeshLoader::LoadData         data;
            std::string                  name;
            Transform                    transform;
            MeshMaterialData             material;
            MeshType                     type{MeshType::Unknown};
            std::optional<PhysicsRecord> physics{std::nullopt};
        };

        struct CachedSubmesh
        {
            MeshHandle     meshHandle;
            MaterialHandle materialHandle;
            glm::vec4      boundingSphere{};
            glm::mat4      localTransform{1.f};
            std::string    name;
        };

        struct CachedModel
        {
            std::vector<CachedSubmesh> submeshes;
            uint32_t                   refCount{};
        };

        AssetPipeline(Context &context,
                      MeshRegistry &meshRegistry,
                      TextureRegistry &textureRegistry,
                      TransferManager &transferManager,
                      Scene &scene,
                      std::span<FrameData> frames);

        void Poll();

        Queue<PendingMeshData> &GetPendingQueue();

        bool IsCached(const std::filesystem::path &path) const;

        std::optional<CachedModel> Uncache(const std::filesystem::path &path);

        using OnLog = std::move_only_function<void(std::string_view)>;
        void SetOnInfoLog(OnLog &&callback);
        void SetOnWarningLog(OnLog &&callback);

        using OnAttachPhysics = std::move_only_function<void(entt::entity, PhysicsRecord)>;
        void SetOnAttachPhysics(OnAttachPhysics &&callback);

    private:
        void ProcessBuiltinMesh(const PendingMeshData &data);

        void ProcessLoadedMesh(const PendingMeshData &data);

        entt::entity CreateParentMeshEntity(const PendingMeshData &data);
        entt::entity CreateSubmeshEntity(
            entt::entity parentEntity,
            const CachedSubmesh &submesh,
            const MeshMaterialData &material
        );

        std::vector<MaterialHandle> LoadAndRegisterMaterials(
            std::span<const TextureRegistry::MaterialData> materials);

        MaterialHandle UploadMaterialDataToRegistry(const TextureRegistry::MaterialData &data);

        static DescriptorSetUpdateInfo make_texture_write(TextureHandle handle, const Texture& texture);

        std::reference_wrapper<Context>         mContext;
        std::reference_wrapper<MeshRegistry>    mMeshRegistry;
        std::reference_wrapper<TextureRegistry> mTextureRegistry;
        std::reference_wrapper<TransferManager> mTransferManager;
        std::reference_wrapper<Scene>           mScene;
        std::span<FrameData>                    mFrames;

        Queue<PendingMeshData>                     mPendingMeshData;

        std::unordered_map<std::string, CachedModel> mMeshCache;

        OnLog mOnInfoLog;
        OnLog mOnWarningLog;

        OnAttachPhysics mOnAttachPhysics;
    };
}
