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
    EntityEditorPanel::EntityEditorPanel() : mSelectedEntity(entt::null),
                                             mRotationDegrees({}),
                                             mCurrentGizmoOperation(ImGuizmo::TRANSLATE),
                                             mCurrentGizmoMode(ImGuizmo::LOCAL),
                                             mUniformScale(true),
                                             mGizmoInUse(false),
                                             mGizmoWasDragging(false),
                                             mSimulationRunning(false)
    {
        mOpen = false;
    }

    EntityEditorPanel::EntityEditorPanel(std::string_view name, ImVec4 backgroundColor)
        : Panel(name, backgroundColor),
          mSelectedEntity(entt::null),
          mRotationDegrees({}),
          mCurrentGizmoOperation(ImGuizmo::TRANSLATE),
          mCurrentGizmoMode(ImGuizmo::LOCAL),
          mUniformScale(true),
          mGizmoInUse(false),
          mGizmoWasDragging(false),
          mSimulationRunning(false)
    {
        mOpen = false;
    }

    void EntityEditorPanel::render(Scene &scene)
    {
        if (!mOpen || mSelectedEntity == entt::null)
            return;
        auto &registry = scene.getEntityRegistry();
        if (!registry.valid(mSelectedEntity))
            return;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, mBackgroundColor);

        if (ImGui::Begin(mName.c_str(), &mOpen))
        {
            const auto &tag = registry.get<TagComponent>(mSelectedEntity);
            ImGui::Text("Entity: %s", tag.name.c_str());
            ImGui::Separator();

            if (registry.all_of<TransformComponent>(mSelectedEntity) &&
                !registry.any_of<PointLightData>(mSelectedEntity))
                renderMeshProperties(registry);

            if (registry.all_of<PhysicsComponent, PhysicsControlComponent>(mSelectedEntity))
                renderBodyProperties(registry);

            if (registry.all_of<MeshMaterialData>(mSelectedEntity))
                renderMaterialProperties(scene);

            else if (registry.all_of<DirectionalLightData>(mSelectedEntity))
                renderDirectionalLightProperties(registry);

            else if (registry.all_of<PointLightData>(mSelectedEntity))
                renderPointLightProperties(registry);

            else if (registry.all_of<CameraComponent>(mSelectedEntity))
                renderCameraProperties(registry);
        }
        ImGui::End();
        ImGui::PopStyleColor();

        renderGizmo(scene);
    }

    void EntityEditorPanel::setSelectedEntity(entt::entity entity, const Scene &scene)
    {
        mSelectedEntity = entity;
        if (entity == entt::null)
            mOpen = false;

        if (scene.getEntityRegistry().all_of<TransformComponent>(mSelectedEntity))
        {
            const auto &transform = scene.getEntityRegistry().get<TransformComponent>(mSelectedEntity);
            mRotationDegrees = glm::degrees(glm::eulerAngles(transform.transform.rotation));
        }
    }

    bool EntityEditorPanel::isGizmoInUse() const
    {
        return mGizmoInUse;
    }

    void EntityEditorPanel::setSimulationState(bool running)
    {
        mSimulationRunning = running;
    }

    void EntityEditorPanel::setOnBodyTypeChange(OnBodyTypeChange &&callback)
    {
        mOnBodyTypeChange = std::move(callback);
    }

    void EntityEditorPanel::setOnBodyScaleChange(OnBodyScaleChange &&callback)
    {
        mOnBodyScaleChange = std::move(callback);
    }

    void EntityEditorPanel::renderGizmo(Scene &scene)
    {
        auto &registry = scene.getEntityRegistry();

        if (!registry.all_of<TransformComponent>(mSelectedEntity))
            return;
        bool isMesh = registry.all_of<MeshComponent>(mSelectedEntity);
        auto operation = isMesh ? mCurrentGizmoOperation : ImGuizmo::TRANSLATE;

        auto &transformComp = registry.get<TransformComponent>(mSelectedEntity);
        auto &transform = transformComp.transform;

        auto modelMatrix = transformComp.worldMatrix;
        const auto &cameraData = registry.get<CameraData>(scene.getMainCamera());

        ImGuizmo::Manipulate(
            glm::value_ptr(cameraData.view),
            glm::value_ptr(cameraData.projection),
            operation,
            mCurrentGizmoMode,
            glm::value_ptr(modelMatrix)
        );

        bool isDragging = ImGuizmo::IsUsing();
        if (mGizmoWasDragging && !isDragging)
            if (operation == ImGuizmo::SCALE &&
                registry.all_of<PhysicsComponent>(mSelectedEntity) &&
                registry.all_of<TransformComponent>(mSelectedEntity))
                mOnBodyScaleChange(
                    registry.get<PhysicsComponent>(mSelectedEntity),
                    registry.get<TransformComponent>(mSelectedEntity).transform.scale
                    );

        mGizmoWasDragging = isDragging;

        mGizmoInUse = isDragging || ImGuizmo::IsOver();
        if (isDragging)
        {
            glm::vec3 translation, scale, skew;
            glm::quat rotation;
            glm::vec4 perspective;

            glm::mat4 parentWorld{1.0f};
            if (auto* hierarchy = registry.try_get<HierarchyComponent>(mSelectedEntity))
                if (hierarchy->parent != entt::null)
                    parentWorld = registry.get<TransformComponent>(hierarchy->parent).worldMatrix;

            auto localMatrix = glm::inverse(parentWorld) * modelMatrix;

            glm::decompose(localMatrix, scale, rotation, translation, skew, perspective);

            transform.position = translation;
            transform.rotation = rotation;
            if (mUniformScale && operation == ImGuizmo::SCALE)
            {
                float avgScale = (scale.x + scale.y + scale.z) / 3.f;
                transform.scale = glm::vec3(avgScale);
            } else
                transform.scale = scale;

            mRotationDegrees = glm::degrees(glm::eulerAngles(transform.rotation));
        }
    }

    void EntityEditorPanel::renderMeshProperties(entt::registry &registry)
    {
        ImGui::Text("Gizmo Operation:");
                if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
                    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
                ImGui::SameLine();
                if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
                    mCurrentGizmoOperation = ImGuizmo::ROTATE;
                ImGui::SameLine();
                if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
                    mCurrentGizmoOperation = ImGuizmo::SCALE;

                ImGui::Text("Gizmo Mode:");
                if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
                    mCurrentGizmoMode = ImGuizmo::LOCAL;
                ImGui::SameLine();
                if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
                    mCurrentGizmoMode = ImGuizmo::WORLD;

                ImGui::Separator();

                auto &transform = registry.get<TransformComponent>(mSelectedEntity).transform;
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::InputFloat3("Translation", glm::value_ptr(transform.position));

                    if (ImGui::InputFloat3("Rotation (Deg)", glm::value_ptr(mRotationDegrees)))
                        transform.rotation = glm::quat(glm::radians(mRotationDegrees));

                    auto oldScale = transform.scale;
                    if (ImGui::InputFloat3("Scale", glm::value_ptr(transform.scale)))
                    {
                        if (mUniformScale)
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
                    if (ImGui::IsItemDeactivatedAfterEdit() && registry.all_of<PhysicsComponent>(mSelectedEntity))
                        mOnBodyScaleChange(registry.get<PhysicsComponent>(mSelectedEntity), transform.scale);
                    ImGui::SameLine();
                    ImGui::Checkbox("##uniform", &mUniformScale);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Uniform Scale");
                }
    }

    void EntityEditorPanel::renderBodyProperties(entt::registry &registry)
    {
        if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &physics = registry.get<PhysicsComponent>(mSelectedEntity);
            auto &control = registry.get<PhysicsControlComponent>(mSelectedEntity);

            ImGui::BeginDisabled(mSimulationRunning);

            int typeIndex = static_cast<int>(physics.type);
            if (ImGui::Combo("Body type", &typeIndex, HierarchyPanel::s_BodyTypeOptions.data()))
            {
                physics.type = static_cast<PhysicsBodyType>(typeIndex);
                mOnBodyTypeChange(physics, physics.type);
            }

            ImGui::InputFloat3("Velocity", glm::value_ptr(control.velocity));

            ImGui::EndDisabled();

            ImGui::InputFloat3("Force", glm::value_ptr(control.force));
            ImGui::Checkbox("Apply Force", &control.applyForce);

            ImGui::InputFloat3("Impulse", glm::value_ptr(control.impulse));
            control.applyImpulse = ImGui::Button("Apply Impulse");
        }
    }

    void EntityEditorPanel::renderMaterialProperties(Scene &scene) const
    {
        if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &material = scene.getEntityRegistry().get<MeshMaterialData>(mSelectedEntity);
            bool changed = false;

            float &roughness = material.albedoAndRoughness.w;
            changed |= ImGui::SliderFloat("Roughness", &roughness, 0.f, 1.f);

            float &metallic = material.pbrParams.x;
            changed |= ImGui::SliderFloat("Metallic", &metallic, 0.f, 1.f);

            float &ao = material.pbrParams.y;
            changed |= ImGui::SliderFloat("AO", &ao, 0.f, 1.f);

            changed |= ImGui::ColorPicker3("Albedo", glm::value_ptr(material.albedoAndRoughness));

            if (changed)
                propagate_material_to_children(scene, mSelectedEntity, material);
        }
    }

    void EntityEditorPanel::renderDirectionalLightProperties(entt::registry &registry) const
    {
        auto &data = registry.get<DirectionalLightData>(mSelectedEntity);
        if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat3("Direction", glm::value_ptr(data.directionAndIntensity), -1.f, 1.f);
            float &intensity = data.directionAndIntensity.w;
            ImGui::InputFloat("Intensity", &intensity);
            ImGui::ColorPicker3("Color", glm::value_ptr(data.colorAndEnabled));
            float &enableValue = data.colorAndEnabled.w;
            bool enabled = enableValue > 0.5f;
            if (ImGui::Checkbox("Enabled", &enabled))
                enabled ? enableValue = 1.f : enableValue = 0.f;
        }
    }

    void EntityEditorPanel::renderPointLightProperties(entt::registry &registry) const
    {
        auto [light, transform] = registry.get<PointLightData, TransformComponent>(mSelectedEntity);
        if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::InputFloat3("Position", glm::value_ptr(transform.transform.position));

            float &intensity = light.positionAndIntensity.w;
            ImGui::InputFloat("Intensity", &intensity);

            float &range = light.range.x;
            ImGui::InputFloat("Range", &range);

            ImGui::ColorPicker3("Color", glm::value_ptr(light.colorAndEnabled));
            registry.get<GizmoComponent>(mSelectedEntity).color = {glm::vec3(light.colorAndEnabled), 1.f};

            float &enableValue = light.colorAndEnabled.w;
            bool enabled = enableValue > 0.5f;
            if (ImGui::Checkbox("Enabled", &enabled))
                enableValue = enabled ? 1.f : 0.f;
        }
    }

    void EntityEditorPanel::renderCameraProperties(entt::registry &registry) const
    {
        auto &camera = registry.get<CameraComponent>(mSelectedEntity);
        if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
            ImGui::InputFloat("Exposure", &camera.exposure, 0.f, 0.f, "%.6f");
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
