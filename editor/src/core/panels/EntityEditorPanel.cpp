#include "EntityEditorPanel.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "HierarchyPanel.h"
#include "core/components/entt/CameraComponent.h"
#include "core/components/entt/HierarchyComponent.h"
#include "core/components/entt/PhysicsControlComponent.h"
#include "core/components/entt/TagComponent.h"
#include "core/components/gpu/CameraData.h"
#include "core/components/gpu/TransformComponent.h"

namespace kailux
{
    EntityEditorPanel::EntityEditorPanel() : m_SelectedEntity(entt::null),
                                             m_RotationDegrees({}),
                                             m_CurrentGizmoOperation(ImGuizmo::TRANSLATE),
                                             m_CurrentGizmoMode(ImGuizmo::LOCAL),
                                             m_UniformScale(true),
                                             m_GizmoInUse(false),
                                             m_GizmoWasDragging(false),
                                             m_SimulationRunning(false)
    {
        m_Open = false;
    }

    EntityEditorPanel::EntityEditorPanel(std::string_view name, ImVec4 backgroundColor)
        : Panel(name, backgroundColor),
          m_SelectedEntity(entt::null),
          m_RotationDegrees({}),
          m_CurrentGizmoOperation(ImGuizmo::TRANSLATE),
          m_CurrentGizmoMode(ImGuizmo::LOCAL),
          m_UniformScale(true),
          m_GizmoInUse(false),
          m_GizmoWasDragging(false),
          m_SimulationRunning(false)
    {
        m_Open = false;
    }

    void EntityEditorPanel::render(Scene &scene)
    {
        if (!m_Open || m_SelectedEntity == entt::null)
            return;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, m_BackgroundColor);

        auto &registry = scene.getEntityRegistry();
        if (ImGui::Begin(m_Name.c_str(), &m_Open))
        {
            const auto &tag = registry.get<TagComponent>(m_SelectedEntity);
            ImGui::Text("Entity: %s", tag.name.c_str());
            ImGui::Separator();

            if (registry.all_of<TransformComponent>(m_SelectedEntity))
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

                auto &transform = registry.get<TransformComponent>(m_SelectedEntity).transform;
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
                    if (ImGui::IsItemDeactivatedAfterEdit() && registry.all_of<PhysicsComponent>(m_SelectedEntity))
                        m_OnBodyScaleChange(registry.get<PhysicsComponent>(m_SelectedEntity), transform.scale);
                    ImGui::SameLine();
                    ImGui::Checkbox("##uniform", &m_UniformScale);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Uniform Scale");
                }
            } if (registry.all_of<PhysicsComponent, PhysicsControlComponent>(m_SelectedEntity))
            {
                if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto &physics = registry.get<PhysicsComponent>(m_SelectedEntity);
                    auto &control = registry.get<PhysicsControlComponent>(m_SelectedEntity);

                    ImGui::BeginDisabled(m_SimulationRunning);

                    int typeIndex = static_cast<int>(physics.type);
                    if (ImGui::Combo("Body type", &typeIndex, HierarchyPanel::s_BodyTypeOptions.data()))
                    {
                        physics.type = static_cast<PhysicsBodyType>(typeIndex);
                        m_OnBodyTypeChange(physics, physics.type);
                    }

                    ImGui::InputFloat3("Velocity", glm::value_ptr(control.velocity));

                    ImGui::EndDisabled();

                    ImGui::InputFloat3("Force", glm::value_ptr(control.force));
                    ImGui::Checkbox("Apply Force", &control.applyForce);

                    ImGui::InputFloat3("Impulse", glm::value_ptr(control.impulse));
                    control.applyImpulse = ImGui::Button("Apply Impulse");
                }
            }
            if (registry.all_of<MeshMaterialData>(m_SelectedEntity))
            {
                if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto &material = registry.get<MeshMaterialData>(m_SelectedEntity);
                    bool changed = false;

                    float &roughness = material.albedoAndRoughness.w;
                    changed |= ImGui::SliderFloat("Roughness", &roughness, 0.f, 1.f);

                    float &metallic = material.pbrParams.x;
                    changed |= ImGui::SliderFloat("Metallic", &metallic, 0.f, 1.f);

                    float &ao = material.pbrParams.y;
                    changed |= ImGui::SliderFloat("AO", &ao, 0.f, 1.f);

                    changed |= ImGui::ColorPicker3("Albedo", glm::value_ptr(material.albedoAndRoughness));

                    if (changed)
                        propagate_material_to_children(scene, m_SelectedEntity, material);
                }
            } else if (registry.all_of<DirectionalLightData>(m_SelectedEntity))
            {
                auto &data = registry.get<DirectionalLightData>(m_SelectedEntity);
                if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::SliderFloat3("Direction", glm::value_ptr(data.directionAndIntensity), -1.f, 1.f);
                    float &intensity = data.directionAndIntensity.w;
                    ImGui::InputFloat("Intensity", &intensity);
                    ImGui::ColorPicker3("Color", glm::value_ptr(data.colorAndEnabled));
                    float &enableValue = data.colorAndEnabled.w;
                    static bool enabled = true;
                    enabled = enableValue > 0.5f;
                    if (ImGui::Checkbox("Enabled", &enabled))
                        enabled ? enableValue = 1.f : enableValue = 0.f;
                }
            } else if (registry.all_of<CameraComponent>(m_SelectedEntity))
            {
                auto &camera = registry.get<CameraComponent>(m_SelectedEntity);
                if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
                    ImGui::InputFloat("Exposure", &camera.exposure, 0.f, 0.f, "%.6f");
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();

        renderGizmo(scene);
    }

    void EntityEditorPanel::setSelectedEntity(entt::entity entity, const Scene &scene)
    {
        m_SelectedEntity = entity;
        if (entity == entt::null)
            m_Open = false;

        if (scene.getEntityRegistry().all_of<TransformComponent>(m_SelectedEntity))
        {
            const auto &transform = scene.getEntityRegistry().get<TransformComponent>(m_SelectedEntity);
            m_RotationDegrees = glm::degrees(glm::eulerAngles(transform.transform.rotation));
        }
    }

    bool EntityEditorPanel::isGizmoInUse() const
    {
        return m_GizmoInUse;
    }

    void EntityEditorPanel::setSimulationState(bool running)
    {
        m_SimulationRunning = running;
    }

    void EntityEditorPanel::setOnBodyTypeChange(OnBodyTypeChange &&callback)
    {
        m_OnBodyTypeChange = std::move(callback);
    }

    void EntityEditorPanel::setOnBodyScaleChange(OnBodyScaleChange &&callback)
    {
        m_OnBodyScaleChange = std::move(callback);
    }

    void EntityEditorPanel::renderGizmo(Scene &scene)
    {
        auto &registry = scene.getEntityRegistry();

        if (!registry.all_of<TransformComponent>(m_SelectedEntity))
            return;

        auto &transformComp = registry.get<TransformComponent>(m_SelectedEntity);
        auto &transform = transformComp.transform;

        auto modelMatrix = transformComp.worldMatrix;
        const auto &cameraData = registry.get<CameraData>(scene.getMainCamera());

        ImGuizmo::Manipulate(
            glm::value_ptr(cameraData.view),
            glm::value_ptr(cameraData.projection),
            m_CurrentGizmoOperation,
            m_CurrentGizmoMode,
            glm::value_ptr(modelMatrix)
        );

        bool isDragging = ImGuizmo::IsUsing();
        if (m_GizmoWasDragging && !isDragging)
            if (m_CurrentGizmoOperation == ImGuizmo::SCALE &&
                registry.all_of<PhysicsComponent>(m_SelectedEntity) &&
                registry.all_of<TransformComponent>(m_SelectedEntity))
                m_OnBodyScaleChange(
                    registry.get<PhysicsComponent>(m_SelectedEntity),
                    registry.get<TransformComponent>(m_SelectedEntity).transform.scale
                    );

        m_GizmoWasDragging = isDragging;

        m_GizmoInUse = isDragging || ImGuizmo::IsOver();
        if (isDragging)
        {
            glm::vec3 translation, scale, skew;
            glm::quat rotation;
            glm::vec4 perspective;

            glm::mat4 parentWorld{1.0f};
            if (auto* hierarchy = registry.try_get<HierarchyComponent>(m_SelectedEntity))
                if (hierarchy->parent != entt::null)
                    parentWorld = registry.get<TransformComponent>(hierarchy->parent).worldMatrix;

            auto localMatrix = glm::inverse(parentWorld) * modelMatrix * glm::inverse(transformComp.submeshLocalMatrix);

            glm::decompose(localMatrix, scale, rotation, translation, skew, perspective);

            transform.position = translation;
            transform.rotation = rotation;
            if (m_UniformScale && m_CurrentGizmoOperation == ImGuizmo::SCALE)
            {
                float avgScale = (scale.x + scale.y + scale.z) / 3.f;
                transform.scale = glm::vec3(avgScale);
            } else
                transform.scale = scale;

            m_RotationDegrees = glm::degrees(glm::eulerAngles(transform.rotation));
        }
    }

    void EntityEditorPanel::propagate_material_to_children(Scene &scene, entt::entity entity,
        const MeshMaterialData &material)
    {
        auto& registry = scene.getEntityRegistry();
        auto* hierarchy = registry.try_get<HierarchyComponent>(entity);
        if (!hierarchy)
            return;

        for (auto child : hierarchy->children)
        {
            if (auto* childMaterial = registry.try_get<MeshMaterialData>(child))
                *childMaterial = material;

            propagate_material_to_children(scene, child, material);
        }
    }
}
