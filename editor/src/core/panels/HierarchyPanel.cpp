#include "HierarchyPanel.h"

#include <imgui_internal.h>

#include "core/components/entt/HierarchyComponent.h"
#include "core/components/entt/MeshComponent.h"
#include "core/components/entt/TagComponent.h"
#include "project_panel/ProjectPanel.h"

namespace kailux
{
    HierarchyPanel::HierarchyPanel() : m_SelectedEntity(entt::null)
    {
    }

    HierarchyPanel::HierarchyPanel(std::string_view name, ImVec4 backgroundColor)
        : Panel(name, backgroundColor), m_SelectedEntity(entt::null)
    {
    }

    void HierarchyPanel::render(Scene &scene)
    {
        if (m_SelectedEntity != m_LastSelectedEntity)
        {
            m_LastSelectedEntity = m_SelectedEntity;
            m_OnEntitySelected(m_SelectedEntity, scene);
        }

        ImGui::PushStyleColor(ImGuiCol_WindowBg, m_BackgroundColor);

        if (ImGui::Begin(m_Name.c_str(), &m_Open))
        {
            auto &registry = scene.getEntityRegistry();

            auto view = registry.view<TagComponent>();
            for (auto entity: view)
            {
                auto *hierarchy = registry.try_get<HierarchyComponent>(entity);

                if (!hierarchy || hierarchy->parent == entt::null)
                    renderEntityNode(scene, entity);
            }

            if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
            {
                m_SelectedEntity = entt::null;
                if (m_OnEntitySelected)
                    m_OnEntitySelected(m_SelectedEntity, scene);
            }

            if (ImGui::BeginPopupContextWindow("##hierarchy_options",
                                               ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                if (ImGui::BeginMenu("New"))
                {
                    if (ImGui::BeginMenu("Mesh"))
                    {
                        if (ImGui::MenuItem("Cube"))
                            m_OnNewMesh(MeshType::Cube);
                        if (ImGui::MenuItem("Sphere"))
                            m_OnNewMesh(MeshType::Sphere);

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Light"))
                    {
                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }

                ImGui::EndPopup();
            }
        }


        if (ImGui::BeginDragDropTargetCustom(
            ImGui::GetCurrentWindow()->InnerRect,
            ImGui::GetID(m_Name.c_str())
        ))
        {
            if (const auto *payload = ImGui::AcceptDragDropPayload(AssetBrowser::s_DragDropPayloadType.data()))
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

    void HierarchyPanel::setOnNewMesh(OnNewMesh &&callback)
    {
        m_OnNewMesh = std::move(callback);
    }

    void HierarchyPanel::selectEntity(entt::entity entity)
    {
        m_SelectedEntity = entity;
    }

    entt::entity HierarchyPanel::getSelectedEntity() const
    {
        return m_SelectedEntity;
    }

    bool HierarchyPanel::on_entity_rename(entt::registry &registry, entt::entity entity)
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

    bool HierarchyPanel::on_entity_delete(const Scene &scene, entt::entity entity)
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

    void HierarchyPanel::notifyAndDestroyHierarchy(entt::registry &registry, entt::entity entity)
    {
        auto *hierarchy = registry.try_get<HierarchyComponent>(entity);

        auto *meshComponent = registry.try_get<MeshComponent>(entity);
        auto *materialComponent = registry.try_get<MaterialComponent>(entity);

        if (meshComponent && materialComponent)
        {
            uint32_t submeshIndex{~0u};

            if (hierarchy && hierarchy->parent != entt::null)
                if (auto *parentHierarchy = registry.try_get<HierarchyComponent>(hierarchy->parent))
                {
                    auto it = std::ranges::find(parentHierarchy->children, entity);
                    if (it != parentHierarchy->children.end())
                        submeshIndex = static_cast<uint32_t>(std::distance(parentHierarchy->children.begin(), it));
                }

            auto submeshCacheKey = std::format("{}_sub{}", meshComponent->path, submeshIndex);

            m_OnEntityDeleted(*meshComponent, *materialComponent, submeshCacheKey);
        }

        if (hierarchy)
            for (auto child: hierarchy->children)
                if (registry.valid(child))
                    notifyAndDestroyHierarchy(registry, child);

        registry.destroy(entity);
    }

    void HierarchyPanel::renderEntityNode(Scene &scene, entt::entity entity)
    {
        auto &registry = scene.getEntityRegistry();
        const auto &tag = registry.get<TagComponent>(entity);
        auto *hierarchy = registry.try_get<HierarchyComponent>(entity);

        ImGuiTreeNodeFlags flags = ((m_SelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0);
        flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (!hierarchy || hierarchy->children.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;

        bool opened = ImGui::TreeNodeEx(reinterpret_cast<void *>(static_cast<uintptr_t>(entity)), flags, "%s",
                                        tag.name.c_str());

        if (ImGui::IsItemClicked())
            m_SelectedEntity = entity;

        if (ImGui::BeginPopupContextItem())
        {
            static entt::entity lastEntity = entt::null;
            static bool nameExistsError = false;

            if (ImGui::IsWindowAppearing() || lastEntity != entity)
            {
                lastEntity = entity;
                nameExistsError = false;
            }

            nameExistsError = on_entity_rename(registry, entity);

            if (nameExistsError)
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Name already exists!");

            ImGui::Separator();

            if (on_entity_delete(scene, entity))
            {
                notifyAndDestroyHierarchy(registry, entity);

                if (m_SelectedEntity == entity)
                {
                    m_SelectedEntity = entt::null;
                    m_LastSelectedEntity = entt::null;
                    m_OnEntitySelected(m_SelectedEntity, scene);
                }
                ImGui::EndPopup();

                if (opened)
                    ImGui::TreePop();
                return;
            }
            ImGui::EndPopup();
        }

        if (opened)
        {
            if (hierarchy)
                for (auto child: hierarchy->children)
                    if (registry.valid(child))
                        renderEntityNode(scene, child);

            ImGui::TreePop();
        }
    }
}
