#pragma once
#include "Panel.h"

#include "texture/TextureRegistry.h"

namespace kailux
{
    class MaterialPanel final : public Panel
    {
    public:
        MaterialPanel();
        MaterialPanel(std::string_view name, ImVec4 backgroundColor);

        void Render(Scene &scene) override;

        void SetSelectedEntity(entt::entity entity);

        using OnSetTexture = std::move_only_function<void(entt::entity, TextureType, const std::filesystem::path &)>;
        void  SetOnSetTexture(OnSetTexture &&callback);

        using OnQueryPreview = std::move_only_function<ImTextureID(entt::entity, TextureType)>;
        void  SetOnQueryPreview(OnQueryPreview &&callback);

    private:
        static constexpr float kThumbnailSize{56.f};

        void RenderHeader(const entt::registry &registry, entt::entity root) const;
        void RenderSurfaceProperties(Scene &scene) const;
        void RenderTextureSlots(const entt::registry &registry, entt::entity root);
        void RenderTextureSlot(const entt::registry &registry, entt::entity root, TextureType type);

        static void propagate_material_to_children(Scene &scene, entt::entity entity, const MeshMaterialData &material);

        entt::entity   mSelectedEntity{entt::null};

        OnSetTexture   mOnSetTexture;
        OnQueryPreview mOnQueryPreview;
    };
}