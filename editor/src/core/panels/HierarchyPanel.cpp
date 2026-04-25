#include "HierarchyPanel.h"

#include <imgui_internal.h>

#include "core/components/entt/MeshComponent.h"
#include "core/components/entt/TagComponent.h"
#include "AssetBrowserPanel.h"

namespace kailux
{
    HierarchyPanel::HierarchyPanel() : m_SelectedEntity(entt::null)
    {
    }

    HierarchyPanel::HierarchyPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor)
        : Panel(name, position, size, backgroundColor), m_SelectedEntity(entt::null)
    {
    }

    void HierarchyPanel::render(Scene &scene)
    {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();

        ImVec2 pos(
            viewport->Pos.x + (m_Position.x * viewport->Size.x),
            viewport->Pos.y + (m_Position.y * viewport->Size.y)
        );
        ImVec2 size(
            m_Size.x * viewport->Size.x,
            m_Size.y * viewport->Size.y
        );

        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, m_BackgroundColor);

        if (ImGui::Begin(m_Name.c_str(), &m_Open,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            auto &registry = scene.getEntityRegistry();

            auto view = registry.view<TagComponent>();
            for (auto entity: view)
            {
                const auto &tag = view.get<TagComponent>(entity);
                bool isSelected = (m_SelectedEntity == entity);
                if (ImGui::Selectable(tag.name.c_str(), isSelected))
                {
                    m_SelectedEntity = entity;
                    m_OnEntitySelected(m_SelectedEntity, scene);
                }

                if (ImGui::BeginPopupContextItem())
                {
                    static entt::entity lastEntity = entt::null;
                    static bool nameExistsError = false;

                    if (ImGui::IsWindowAppearing() || lastEntity != entity)
                    {
                        lastEntity = entity;
                        nameExistsError = false;
                    }
                    nameExistsError = on_entity_rename(entity, registry);

                    if (nameExistsError)
                        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Name already exists!");
                    ImGui::Separator();

                    if (on_entity_delete(entity, scene))
                    {
                        auto [meshComponent, materialComponent] = registry.get<MeshComponent, MaterialComponent>(entity);
                        m_OnEntityDeleted(meshComponent, materialComponent);
                        registry.destroy(entity);
                        if (m_SelectedEntity == entity)
                        {
                            m_SelectedEntity = entt::null;
                            m_OnEntitySelected(entt::null, scene);
                        }
                        ImGui::EndPopup();
                        break;
                    }

                    if (ImGui::Button("Cancel"))
                    {
                        nameExistsError = false;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }
            }
        }


        if (ImGui::BeginDragDropTargetCustom(
            ImGui::GetCurrentWindow()->InnerRect,
            ImGui::GetID(m_Name.c_str())
        ))
        {
            if (const auto *payload = ImGui::AcceptDragDropPayload(AssetBrowserPanel::s_DragDropPayloadType.data()))
            {
                std::string path = static_cast<const char *>(payload->Data);
                m_OnDragDrop(path);
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::End();
        ImGui::PopStyleColor();
    }

    void HierarchyPanel::setOnEntitySelected(OnEntitySelected &&callback)
    {
        m_OnEntitySelected = std::move(callback);
    }

    void HierarchyPanel::setOnEntityDeleted(OnEntityDeleted &&callback)
    {
        m_OnEntityDeleted = std::move(callback);
    }

    void HierarchyPanel::setOnDragDrop(OnDragDrop &&callback)
    {
        m_OnDragDrop = std::move(callback);
    }

    bool HierarchyPanel::on_entity_rename(entt::entity entity, entt::registry &registry)
    {
        static char nameBuffer[64]{};
        static bool nameExistsError = false;

        if (ImGui::IsWindowAppearing())
        {
            nameExistsError = false;
            std::strncpy(nameBuffer, registry.get<TagComponent>(entity).name.c_str(), sizeof(nameBuffer) - 1);
            nameBuffer[sizeof(nameBuffer) - 1] = 0;
        }

        ImGui::Text("Rename Entity");

        if (ImGui::InputText("##rename", nameBuffer, sizeof(nameBuffer),
                             ImGuiInputTextFlags_EnterReturnsTrue))
        {
            std::string newName(nameBuffer);

            bool foundDuplicate = false;
            auto view = registry.view<TagComponent>();
            for (auto otherEntity: view)
            {
                if (otherEntity != entity &&
                    registry.get<TagComponent>(otherEntity).name == newName)
                {
                    foundDuplicate = true;
                    break;
                }
            }

            if (!newName.empty() && !foundDuplicate)
            {
                registry.get<TagComponent>(entity).name = newName;
                nameExistsError = false;
                ImGui::CloseCurrentPopup();
            } else
                nameExistsError = foundDuplicate;
        }
        return nameExistsError;
    }

    bool HierarchyPanel::on_entity_delete(entt::entity entity, Scene &scene)
    {
        if (ImGui::MenuItem("Delete"))
        {
            if (entity != scene.getSun())
            {
                ImGui::CloseCurrentPopup();
                return true;
            }
            ImGui::CloseCurrentPopup();
        }
        return false;
    }
}
