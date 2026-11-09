#include "MaterialPanel.h"

#include <glm/gtc/type_ptr.hpp>

#include "../UIWidgets.h"
#include "components/entt/HierarchyComponent.h"
#include "components/entt/MaterialTexturesComponent.h"
#include "components/entt/TagComponent.h"
#include "components/gpu/MeshMaterialData.h"
#include "project_panel/AssetBrowser.h"
#include "texture/MaterialTextureSystem.h"

namespace kailux
{
    MaterialPanel::MaterialPanel() = default;

    MaterialPanel::MaterialPanel(std::string_view name, ImVec4 backgroundColor) : Panel(name, backgroundColor)
    {
    }

    void MaterialPanel::Render(Scene &scene)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, mBackgroundColor);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.f, 14.f));
        const bool visible{ImGui::Begin(mName.c_str(), &mOpen)};
        ImGui::PopStyleVar();
        mFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        if (visible)
        {
            const auto &registry{scene.GetEntityRegistry()};
            const bool  hasSelection{mSelectedEntity != entt::null && registry.valid(mSelectedEntity)};
            const auto  root{
                hasSelection
                    ? MaterialTextureSystem::find_material_root(registry, mSelectedEntity)
                    : entt::null
            };

            if (root == entt::null)
                ImGui::TextDisabled("Select a mesh to edit its material");
            else
            {
                RenderHeader(registry, root);

                ImGui::BeginDisabled(mLocked);
                if (registry.all_of<MeshMaterialData>(mSelectedEntity))
                    RenderSurfaceProperties(scene);
                RenderTextureSlots(registry, root);
                ImGui::EndDisabled();
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();
    }

    void MaterialPanel::SetSelectedEntity(entt::entity entity)
    {
        mSelectedEntity = entity;
    }

    void MaterialPanel::SetOnSetTexture(OnSetTexture &&callback)
    {
        mOnSetTexture = std::move(callback);
    }

    void MaterialPanel::SetOnQueryPreview(OnQueryPreview &&callback)
    {
        mOnQueryPreview = std::move(callback);
    }

    void MaterialPanel::RenderHeader(const entt::registry &registry, entt::entity root) const
    {
        const auto &[name]{registry.get<TagComponent>(mSelectedEntity)};

        ImGui::SetWindowFontScale(1.15f);
        ImGui::TextUnformatted(name.c_str());
        ImGui::SetWindowFontScale(1.f);

        if (root == mSelectedEntity)
            ImGui::TextDisabled("Material");
        else
            ImGui::TextDisabled("Textures apply to the whole model '%s'",
                                registry.get<TagComponent>(root).name.c_str());
        ImGui::Dummy(ImVec2(0.f, 4.f));
    }

    void MaterialPanel::RenderSurfaceProperties(Scene &scene) const
    {
        if (!widgets::section("Surface"))
            return;

        auto &material = scene.GetEntityRegistry().get<MeshMaterialData>(mSelectedEntity);
        bool changed = false;

        if (!widgets::begin_properties("##material"))
            return;

        widgets::property("Albedo");
        changed |= ImGui::ColorEdit3("##albedo", glm::value_ptr(material.albedoAndRoughness),
                                     ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_PickerHueWheel);

        widgets::property("Roughness");
        changed |= ImGui::SliderFloat("##roughness", &material.albedoAndRoughness.w, 0.f, 1.f, "%.2f");

        widgets::property("Metallic");
        changed |= ImGui::SliderFloat("##metallic", &material.pbrParams.x, 0.f, 1.f, "%.2f");

        widgets::property("AO");
        changed |= ImGui::SliderFloat("##ao", &material.pbrParams.y, 0.f, 1.f, "%.2f");

        widgets::property("Grid");
        bool grid{material.pbrParams.z > 0.f};
        if (ImGui::Checkbox("##grid", &grid))
        {
            material.pbrParams.z = grid ? kDefaultGridCellSize : 0.f;
            changed = true;
        }

        if (grid)
        {
            widgets::property("Grid cell");
            changed |= ImGui::DragFloat("##grid_cell", &material.pbrParams.z, 0.05f, 0.05f, 100.f, "%.2f m",
                                        ImGuiSliderFlags_AlwaysClamp);
        }

        widgets::end_properties();

        if (changed)
            propagate_material_to_children(scene, mSelectedEntity, material);
    }

    void MaterialPanel::RenderTextureSlots(const entt::registry &registry, entt::entity root)
    {
        if (!widgets::section("Textures"))
            return;

        ImGui::TextDisabled("Drop a .png or .jpg from the project on a slot");
        ImGui::Dummy(ImVec2(0.f, 2.f));

        for (const auto type : TextureRegistry::kTextureTypes)
            RenderTextureSlot(registry, root, type);
    }

    void MaterialPanel::RenderTextureSlot(const entt::registry &registry, entt::entity root, TextureType type)
    {
        const auto  index{TextureRegistry::texture_type_index(type)};
        const auto *textures{registry.try_get<MaterialTexturesComponent>(root)};
        const std::string_view path{textures ? std::string_view{textures->paths[index]} : std::string_view{}};

        ImGui::PushID(static_cast<int>(index));

        ImGui::BeginGroup();
        {
            constexpr ImVec2 thumbnail{kThumbnailSize, kThumbnailSize};
            const auto   preview{mOnQueryPreview ? mOnQueryPreview(mSelectedEntity, type) : ImTextureID{}};
            if (preview)
                ImGui::Image(preview, thumbnail);
            else
                ImGui::Dummy(thumbnail);

            const auto min{ImGui::GetItemRectMin()};
            const auto max{ImGui::GetItemRectMax()};
            ImGui::GetWindowDrawList()->AddRect(min, max, widgets::colors::kRowBorder);

            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::TextUnformatted(TextureRegistry::texture_type_name(type).data());
            if (path.empty())
                ImGui::TextDisabled("default");
            else
            {
                const auto fileName{std::filesystem::path{path}.filename().string()};
                ImGui::TextDisabled("%s", fileName.c_str());
                if (ImGui::SmallButton("Clear") && mOnSetTexture)
                    mOnSetTexture(root, type, {});
            }
            ImGui::EndGroup();
        }
        ImGui::EndGroup();

        if (!path.empty())
            ImGui::SetItemTooltip("%s", std::string{path}.c_str());

        if (ImGui::BeginDragDropTarget())
        {
            if (const auto *payload = ImGui::AcceptDragDropPayload(AssetBrowser::s_DragDropPayloadType.data()))
            {
                const std::filesystem::path dropped{static_cast<const char *>(payload->Data)};
                if (mOnSetTexture)
                    mOnSetTexture(root, type, dropped);
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();
        ImGui::Dummy(ImVec2(0.f, 2.f));
    }

    void MaterialPanel::propagate_material_to_children(Scene &scene, entt::entity entity,
                                                       const MeshMaterialData &material)
    {
        auto &registry{scene.GetEntityRegistry()};
        const auto *hierarchy{registry.try_get<HierarchyComponent>(entity)};
        if (!hierarchy)
            return;

        for (const auto child : hierarchy->children)
        {
            if (auto *childMaterial = registry.try_get<MeshMaterialData>(child))
                *childMaterial = material;

            propagate_material_to_children(scene, child, material);
        }
    }
}