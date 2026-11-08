#include "MaterialTextureSystem.h"


#include <format>

#include "components/entt/HierarchyComponent.h"
#include "components/entt/MaterialComponent.h"
#include "components/entt/MaterialTexturesComponent.h"
#include "components/entt/MeshSourceComponent.h"
#include "imgui_backend/ImGuiBackend.h"
#include "passes/MainPass.h"

namespace kailux
{
    MaterialTextureSystem::MaterialTextureSystem(Context &context,
                                                 TextureRegistry &textureRegistry,
                                                 TransferManager &transferManager,
                                                 std::span<FrameData> frames,
                                                 Eraser &eraser)
        : mContext(context),
          mTextureRegistry(textureRegistry),
          mTransferManager(transferManager),
          mFrames(frames),
          mEraser(eraser),
          mOnTexturesFreed([](auto) {}),
          mOnWarningLog([](auto) {})
    {
    }

    void MaterialTextureSystem::SetThreadDispatcher(ThreadDispatcher *dispatcher)
    {
        pThreadDispatcher = dispatcher;
    }

    void MaterialTextureSystem::SetOnTexturesFreed(OnTexturesFreed &&callback)
    {
        mOnTexturesFreed = std::move(callback);
    }

    void MaterialTextureSystem::SetOnWarningLog(OnLog &&callback)
    {
        mOnWarningLog = std::move(callback);
    }

    void MaterialTextureSystem::AttachScene(Scene &scene)
    {
        auto sink{scene.GetEntityRegistry().on_destroy<MaterialComponent>()};
        sink.disconnect<&MaterialTextureSystem::OnMaterialDestroyed>(*this);
        sink.connect<&MaterialTextureSystem::OnMaterialDestroyed>(*this);
    }

    void MaterialTextureSystem::DetachScene(Scene &scene)
    {
        auto &registry{scene.GetEntityRegistry()};
        registry.on_destroy<MaterialComponent>().disconnect<&MaterialTextureSystem::OnMaterialDestroyed>(*this);

        for (const auto entity : registry.view<MaterialComponent>())
        {
            auto &material{registry.get<MaterialComponent>(entity)};
            if (!material.owned)
                continue;
            ReleaseMaterialDeferred(material.handle);
            material.owned = false;
        }
    }

    void MaterialTextureSystem::Request(Scene &scene, entt::entity entity, TextureType type, std::string path)
    {
        auto &registry{scene.GetEntityRegistry()};
        if (!registry.valid(entity))
            return;

        const auto root{find_material_root(registry, entity)};
        if (root == entt::null)
        {
            mOnWarningLog("Material textures can only be set on meshes");
            return;
        }

        const auto index{TextureRegistry::texture_type_index(type)};
        auto &textures{registry.get_or_emplace<MaterialTexturesComponent>(root)};
        textures.paths[index] = std::move(path);
        ++textures.revisions[index];
        textures.pendingMask |= 1u << index;
    }

    void MaterialTextureSystem::Update(Scene &scene)
    {
        ApplyUploaded(scene);
        UploadDecoded(scene);
        StartPendingLoads(scene);
    }

    bool MaterialTextureSystem::IsIdle() const
    {
        std::lock_guard lock{mDecodeState->mutex};
        return mDecodeState->inFlight == 0
               && mDecodeState->decoded.empty()
               && mUploadsInFlight == 0
               && mUploaded.empty();
    }

    ImTextureID MaterialTextureSystem::GetPreview(const Scene &scene, entt::entity entity, TextureType type)
    {
        const auto &registry{scene.GetEntityRegistry()};
        if (!registry.valid(entity))
            return {};

        const auto root{find_material_root(registry, entity)};
        if (root == entt::null)
            return {};

        auto target{registry.all_of<MaterialComponent>(entity) ? entity : entt::null};
        if (target == entt::null)
        {
            const auto targets{collect_targets(registry, root)};
            if (targets.empty())
                return {};
            target = targets.front();
        }

        const auto &textureRegistry{mTextureRegistry.get()};
        const auto texture{
            textureRegistry.GetMaterialTexture(registry.get<MaterialComponent>(target).handle, type)
        };
        if (!texture.Valid())
            return {};

        if (const auto it{mPreviews.find(texture.index)}; it != mPreviews.end())
            return it->second;

        const auto id{ImGuiBackend::get_texture_id_from_texture(textureRegistry.GetTexture(texture))};
        mPreviews.emplace(texture.index, id);
        return id;
    }

    void MaterialTextureSystem::DropPreviews(std::span<const uint32_t> textureSlots)
    {
        for (const auto slot : textureSlots)
            if (const auto it{mPreviews.find(slot)}; it != mPreviews.end())
            {
                ImGuiBackend::remove_texture(it->second);
                mPreviews.erase(it);
            }
    }

    entt::entity MaterialTextureSystem::find_material_root(const entt::registry &registry, entt::entity entity)
    {
        for (auto current{entity}; current != entt::null && registry.valid(current);)
        {
            if (registry.all_of<MeshSourceComponent>(current))
                return current;

            const auto* hierarchy{registry.try_get<HierarchyComponent>(current)};
            current = hierarchy ? hierarchy->parent : entt::null;
        }
        return entt::null;
    }

    std::vector<entt::entity> MaterialTextureSystem::collect_targets(const entt::registry &registry, entt::entity root)
    {
        std::vector<entt::entity> targets;
        std::vector stack{root};
        while (!stack.empty())
        {
            const auto entity{stack.back()};
            stack.pop_back();
            if (!registry.valid(entity))
                continue;

            if (registry.all_of<MaterialComponent>(entity))
                targets.push_back(entity);

            if (const auto *hierarchy{registry.try_get<HierarchyComponent>(entity)})
                stack.insert(stack.end(), hierarchy->children.begin(), hierarchy->children.end());
        }
        return targets;
    }

    void MaterialTextureSystem::StartPendingLoads(Scene &scene)
    {
        auto &registry{scene.GetEntityRegistry()};
        const auto &textureRegistry{mTextureRegistry.get()};

        for (const auto root : registry.view<MaterialTexturesComponent>())
        {
            auto &textures{registry.get<MaterialTexturesComponent>(root)};
            if (textures.pendingMask == 0u)
                continue;

            if (collect_targets(registry, root).empty())
                continue;

            for (uint32_t index{}; index < MaterialTexturesComponent::kCount; ++index)
            {
                const auto bit{1u << index};
                if (!(textures.pendingMask & bit))
                    continue;
                textures.pendingMask &= ~bit;

                const auto type{TextureRegistry::kTextureTypes[index]};
                if (textures.paths[index].empty())
                {
                    ApplyTexture(scene, root, type, textureRegistry.GetDefaultTextureHandle(type));
                    continue;
                }

                Job job{scene.GetId(), root, type, textures.revisions[index], textures.paths[index]};
                {
                    std::lock_guard lock{mDecodeState->mutex};
                    ++mDecodeState->inFlight;
                }

                auto decode = [state = mDecodeState, job = std::move(job)]() mutable
                {
                    auto image{
                        image_loader::load_image(job.path, TextureRegistry::texture_color_space(job.type))
                    };
                    std::lock_guard lock{state->mutex};
                    --state->inFlight;
                    state->decoded.emplace_back(std::move(job), std::move(image));
                };

                if (pThreadDispatcher)
                    pThreadDispatcher->Enqueue(std::move(decode));
                else
                    decode();
            }
        }
    }

    void MaterialTextureSystem::UploadDecoded(Scene &scene)
    {
        std::vector<Decoded> decoded;
        {
            std::lock_guard lock{mDecodeState->mutex};
            std::swap(decoded, mDecodeState->decoded);
        }

        auto &textureRegistry{mTextureRegistry.get()};
        for (auto &[job, image] : decoded)
        {
            if (!IsCurrent(scene, job))
                continue;

            if (!image)
            {
                mOnWarningLog(std::format("Texture '{}' not loaded: {}", job.path, image.error()));
                continue;
            }

            auto texture{textureRegistry.CreateTextureFromData(mContext, *image)};
            if (!texture)
            {
                mOnWarningLog(std::format("Texture '{}' not loaded: no free texture slot", job.path));
                continue;
            }

            const auto &created{textureRegistry.GetTexture(texture->handle)};
            const std::array writes{
                DescriptorSetUpdateInfo{
                    MainPass::kMeshTextureBindStart,
                    texture->handle.index,
                    DescriptorSetImageInfo(
                        created.GetSampler(),
                        created.GetImageView(),
                        vk::ImageLayout::eShaderReadOnlyOptimal,
                        1
                    )
                }
            };
            for (const auto &frame : mFrames)
                frame.GetMeshDescriptorSet().UpdateInfo(mContext, writes);

            std::vector<ImageUpload> uploads{texture->upload};
            std::vector<Buffer>      staging;
            staging.push_back(std::move(texture->staging));

            ++mUploadsInFlight;
            mTransferManager.get().EnqueueImages(
                mContext,
                std::move(uploads),
                std::move(staging),
                [this, job = std::move(job), handle = texture->handle]() mutable
                {
                    --mUploadsInFlight;
                    mUploaded.push_back(Uploaded{std::move(job), handle});
                }
            );
        }
    }

    void MaterialTextureSystem::ApplyUploaded(Scene &scene)
    {
        auto uploaded{std::exchange(mUploaded, {})};
        for (const auto &[job, texture]: uploaded)
            if (!IsCurrent(scene, job)
                || !ApplyTexture(scene, job.root, job.type, texture))
                ReleaseTextureDeferred(texture);
    }

    bool MaterialTextureSystem::IsCurrent(const Scene &scene, const Job &job) const
    {
        if (job.scene != scene.GetId())
            return false;

        const auto &registry{scene.GetEntityRegistry()};
        if (!registry.valid(job.root))
            return false;

        const auto *textures{registry.try_get<MaterialTexturesComponent>(job.root)};
        return textures && textures->revisions[TextureRegistry::texture_type_index(job.type)] == job.revision;
    }

    bool MaterialTextureSystem::ApplyTexture(Scene &scene, entt::entity root, TextureType type, TextureHandle texture)
    {
        auto &registry{scene.GetEntityRegistry()};
        auto &textureRegistry{mTextureRegistry.get()};

        uint32_t applied{};
        for (const auto target : collect_targets(registry, root))
        {
            if (!EnsureOwned(registry, target))
                continue;

            if (applied++ > 0u)
                textureRegistry.AcquireTexture(texture);

            const auto previous{
                textureRegistry.UpdateTexture(registry.get<MaterialComponent>(target).handle, type, texture)
            };
            ReleaseTextureDeferred(previous);
        }
        return applied > 0u;
    }

    bool MaterialTextureSystem::EnsureOwned(entt::registry &registry, entt::entity entity)
    {
        auto &material{registry.get<MaterialComponent>(entity)};
        if (material.owned)
            return true;

        const auto clone{mTextureRegistry.get().CloneMaterial(material.handle)};
        if (!clone)
        {
            mOnWarningLog(std::format("Material limit ({}) reached, the texture was not applied",
                                      details::kMaxMaterials));
            return false;
        }

        material.handle = *clone;
        material.owned  = true;
        return true;
    }

    void MaterialTextureSystem::ReleaseTextureDeferred(TextureHandle texture)
    {
        if (!texture.Valid() || mTextureRegistry.get().IsDefaultTexture(texture))
            return;

        mEraser.get().Enqueue([this, texture]()
        {
            if (mTextureRegistry.get().ReleaseTexture(texture))
            {
                const std::array freed{texture.index};
                mOnTexturesFreed(freed);
            }
        });
    }

    void MaterialTextureSystem::ReleaseMaterialDeferred(MaterialHandle material)
    {
        mEraser.get().Enqueue([this, material]()
        {
            const auto freed{mTextureRegistry.get().ReleaseMaterial(material)};
            mOnTexturesFreed(freed);
        });
    }

    void MaterialTextureSystem::OnMaterialDestroyed(entt::registry &registry, entt::entity entity)
    {
        auto &material{registry.get<MaterialComponent>(entity)};
        if (!material.owned)
            return;

        ReleaseMaterialDeferred(material.handle);
        material.owned = false;
    }
}
