#include "EntityEditorPanel.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "core/components/entt/CameraComponent.h"
#include "core/components/entt/TagComponent.h"

namespace kailux
{
    EntityEditorPanel::EntityEditorPanel() : m_SelectedEntity(entt::null), m_RotationDegrees({}),
                                             m_CurrentGizmoMode(ImGuizmo::LOCAL),
                                             m_CurrentGizmoOperation(ImGuizmo::TRANSLATE),
    m_UniformScale(true)
    {
    }

    EntityEditorPanel::EntityEditorPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor)
        : Panel(name, position, size, backgroundColor),
          m_SelectedEntity(entt::null), m_RotationDegrees({}),
          m_CurrentGizmoMode(ImGuizmo::LOCAL),
          m_CurrentGizmoOperation(ImGuizmo::TRANSLATE),
    m_UniformScale(true)
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

        auto &registry = scene.getEntityRegistry();
        if (ImGui::Begin(m_Name.c_str(), nullptr,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            auto &tag = registry.get<TagComponent>(m_SelectedEntity);
            ImGui::Text("Entity: %s", tag.name.c_str());
            ImGui::Separator();

            if (registry.all_of<MeshTransformData>(m_SelectedEntity))
            {
                ImGui::Text("Gizmo Operation:");
                if (ImGui::RadioButton("Translate", m_CurrentGizmoOperation == ImGuizmo::TRANSLATE))
                    m_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
                ImGui::SameLine();
                if (ImGui::RadioButton("Rotate", m_CurrentGizmoOperation == ImGuizmo::ROTATE))
                    m_CurrentGizmoOperation = ImGuizmo::ROTATE;
                ImGui::SameLine();
                if (ImGui::RadioButton("Scale", m_CurrentGizmoOperation == ImGuizmo::SCALE))
                    m_CurrentGizmoOperation = ImGuizmo::SCALE;

                ImGui::Text("Gizmo Mode:");
                if (ImGui::RadioButton("Local", m_CurrentGizmoMode == ImGuizmo::LOCAL))
                    m_CurrentGizmoMode = ImGuizmo::LOCAL;
                ImGui::SameLine();
                if (ImGui::RadioButton("World", m_CurrentGizmoMode == ImGuizmo::WORLD))
                    m_CurrentGizmoMode = ImGuizmo::WORLD;

                ImGui::Separator();

                auto &transform = registry.get<MeshTransformData>(m_SelectedEntity);
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::InputFloat3("Translation", glm::value_ptr(transform.position));

                    if (ImGui::InputFloat3("Rotation (Deg)", glm::value_ptr(m_RotationDegrees)))
                        transform.rotation = glm::quat(glm::radians(m_RotationDegrees));

                    auto oldScale = transform.scale;
                    if (ImGui::InputFloat3("Scale", glm::value_ptr(transform.scale)))
                    {
                        if (m_UniformScale)
                        {
                            float newValue = oldScale.x;
                            if (transform.scale.x != oldScale.x)
                                newValue = transform.scale.x;
                            else if (transform.scale.y != oldScale.y)
                                newValue = transform.scale.y;
                            else if (transform.scale.z != oldScale.z)
                                newValue = transform.scale.z;
                            transform.scale = glm::vec3(newValue);
                        }
                    }
                    ImGui::SameLine();
                    ImGui::Checkbox("##uniform", &m_UniformScale);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Uniform Scale");
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();

        const auto &camera = registry.get<CameraComponent>(scene.getMainCamera());

        if (!registry.all_of<MeshTransformData>(m_SelectedEntity))
            return;

        auto &transform = registry.get<MeshTransformData>(m_SelectedEntity);

        auto modelMatrix = glm::mat4(1.f);
        modelMatrix = glm::translate(modelMatrix, transform.position);
        modelMatrix *= glm::mat4_cast(transform.rotation);
        modelMatrix = glm::scale(modelMatrix, transform.scale);

        ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
        ImGuizmo::SetRect(
            viewport->Pos.x, viewport->Pos.y,
            viewport->Size.x, viewport->Size.y
        );

        ImGuizmo::Manipulate(
            glm::value_ptr(camera.camera.getView()),
            glm::value_ptr(camera.camera.getProjection()),
            m_CurrentGizmoOperation,
            m_CurrentGizmoMode,
            glm::value_ptr(modelMatrix)
        );

        if (ImGuizmo::IsUsing())
        {
            glm::vec3 translation, scale, skew;
            glm::quat rotation;
            glm::vec4 perspective;

            glm::decompose(modelMatrix, scale, rotation, translation, skew, perspective);

            transform.position = translation;
            transform.rotation = rotation;
            if (m_UniformScale && m_CurrentGizmoOperation == ImGuizmo::SCALE)
            {
                float avgScale = (scale.x + scale.y + scale.z) / 3.f;
                transform.scale = glm::vec3(avgScale);
            }
            else
                transform.scale = scale;

            m_RotationDegrees = glm::degrees(glm::eulerAngles(transform.rotation));
        }
    }

    void EntityEditorPanel::setSelectedEntity(entt::entity entity, const Scene &scene)
    {
        m_SelectedEntity = entity;

        if (scene.getEntityRegistry().all_of<MeshTransformData>(m_SelectedEntity))
        {
            const auto &transform = scene.getEntityRegistry().get<MeshTransformData>(m_SelectedEntity);
            m_RotationDegrees = glm::degrees(glm::eulerAngles(transform.rotation));
        }
    }
}
