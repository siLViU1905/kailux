#include "EntityEditorPanel.h"
#include <glm/gtc/type_ptr.hpp>

#include "core/components/entt/TagComponent.h"

namespace kailux
{
    EntityEditorPanel::EntityEditorPanel() : m_SelectedEntity(entt::null), m_RotationDegrees()
    {
    }

    EntityEditorPanel::EntityEditorPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor)
        : Panel(name, position, size, backgroundColor), m_SelectedEntity(entt::null), m_RotationDegrees()
    {
    }

    void EntityEditorPanel::render(Scene &scene)
    {
        if (m_SelectedEntity == entt::null)
            return;

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

            auto &tag = registry.get<TagComponent>(m_SelectedEntity);
            ImGui::Text("Entity: %s", tag.name.c_str());
            ImGui::Separator();

            if (registry.all_of<MeshTransformData>(m_SelectedEntity))
            {
                auto &transform = registry.get<MeshTransformData>(m_SelectedEntity);
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::SliderFloat3("Translation", glm::value_ptr(transform.position), -100.0f, 100.0f);

                    if (ImGui::SliderFloat3("Rotation", glm::value_ptr(m_RotationDegrees), -180.0f, 180.0f))
                        transform.rotation = glm::quat(glm::radians(m_RotationDegrees));

                    ImGui::SliderFloat3("Scale", glm::value_ptr(transform.scale), 0.1f, 10.0f);
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();
    }

    void EntityEditorPanel::setSelectedEntity(entt::entity entity, const Scene &scene)
    {
        m_SelectedEntity = entity;

        if (scene.getEntityRegistry().all_of<MeshTransformData>(m_SelectedEntity))
        {
            const auto& transform = scene.getEntityRegistry().get<MeshTransformData>(m_SelectedEntity);
            m_RotationDegrees = glm::degrees(glm::eulerAngles(transform.rotation));
        }
    }
}
