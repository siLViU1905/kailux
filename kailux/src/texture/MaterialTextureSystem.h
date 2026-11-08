#pragma once

#include <expected>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <entt/entt.hpp>
#include <imgui.h>

#include "TextureRegistry.h"
#include "Context.h"
#include "DeferredResourceEraser.h"
#include "FrameData.h"
#include "TransferManager.h"
#include "scene/Scene.h"
#include "utilities/ThreadDispatcher.h"

namespace kailux
{
    class MaterialTextureSystem
    {
    public:
        using Eraser = DeferredResourceEraser<details::kFramesInFlight + 1>;

        MaterialTextureSystem(Context &context,
                              TextureRegistry &textureRegistry,
                              TransferManager &transferManager,
                              std::span<FrameData> frames,
                              Eraser &eraser);

        void SetThreadDispatcher(ThreadDispatcher *dispatcher);

        using OnTexturesFreed = std::move_only_function<void(std::span<const uint32_t>)>;
        void SetOnTexturesFreed(OnTexturesFreed &&callback);

        using OnLog = std::move_only_function<void(std::string_view)>;
        void SetOnWarningLog(OnLog &&callback);

        void AttachScene(Scene &scene);
        void DetachScene(Scene &scene);

        void Request(Scene &scene, entt::entity entity, TextureType type, std::string path);

        void Update(Scene &scene);

        bool IsIdle() const;

        ImTextureID GetPreview(const Scene &scene, entt::entity entity, TextureType type);
        void        DropPreviews(std::span<const uint32_t> textureSlots);

        static entt::entity find_material_root(const entt::registry &registry, entt::entity entity);

    private:
        struct Job
        {
            SceneId      scene{kInvalidSceneId};
            entt::entity root{entt::null};
            TextureType  type{TextureType::Albedo};
            uint32_t     revision{};
            std::string  path;
        };

        struct Decoded
        {
            Job                   job;
            image_loader::Result  image;
        };

        struct Uploaded
        {
            Job           job;
            TextureHandle texture;
        };

        struct DecodeState
        {
            std::mutex           mutex;
            std::vector<Decoded> decoded;
            uint32_t             inFlight{};
        };

        static std::vector<entt::entity> collect_targets(const entt::registry &registry, entt::entity root);

        void StartPendingLoads(Scene &scene);
        void UploadDecoded(Scene &scene);
        void ApplyUploaded(Scene &scene);

        bool IsCurrent(const Scene &scene, const Job &job) const;

        bool ApplyTexture(Scene &scene, entt::entity root, TextureType type, TextureHandle texture);
        bool EnsureOwned(entt::registry &registry, entt::entity entity);

        void ReleaseTextureDeferred(TextureHandle texture);
        void ReleaseMaterialDeferred(MaterialHandle material);

        void OnMaterialDestroyed(entt::registry &registry, entt::entity entity);

        std::reference_wrapper<Context>         mContext;
        std::reference_wrapper<TextureRegistry> mTextureRegistry;
        std::reference_wrapper<TransferManager> mTransferManager;
        std::span<FrameData>                    mFrames;
        std::reference_wrapper<Eraser>          mEraser;
        ThreadDispatcher*                       pThreadDispatcher{};

        Shared<DecodeState>                     mDecodeState{create_shared<DecodeState>()};
        std::vector<Uploaded>                   mUploaded;
        uint32_t                                mUploadsInFlight{};

        std::unordered_map<uint32_t, ImTextureID> mPreviews;

        OnTexturesFreed mOnTexturesFreed;
        OnLog           mOnWarningLog;
    };
}