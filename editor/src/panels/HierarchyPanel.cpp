#include "HierarchyPanel.h"

#include "core/components/entt/TagComponent.h"

namespace kailux
{
    HierarchyPanel::HierarchyPanel() : m_SelectedEntity()
    {
    }

    HierarchyPanel::HierarchyPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor)
        : Panel(name, position, size, backgroundColor), m_SelectedEntity()
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

        if (ImGui::Begin(m_Name.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
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
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();
    }

    entt::entity HierarchyPanel::getSelectedEntity() const
    {
        return m_SelectedEntity;
    }
}
