#include "EntityEditorPanel.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "HierarchyPanel.h"
#include "core/components/entt/BuiltinCamera.h"
#include "core/components/entt/CameraComponent.h"
#include "core/components/entt/HierarchyComponent.h"
#include "core/components/entt/LocalTransform.h"
#include "core/components/entt/PhysicsControlComponent.h"
#include "core/components/entt/TagComponent.h"
#include "core/components/entt/WorldTransform.h"
#include "core/components/gpu/CameraData.h"
#include "../UIWidgets.h"

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

    void EntityEditorPanel::Render(Scene &scene)
    {
        if (!mOpen || mSelectedEntity == entt::null)
            return;
        auto &registry = scene.GetEntityRegistry();
        if (!registry.valid(mSelectedEntity))
            return;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, mBackgroundColor);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.f, 14.f));
        const bool visible{ImGui::Begin(mName.c_str(), &mOpen)};
        ImGui::PopStyleVar();
        mFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        if (visible)
        {
            RenderHeader(registry);

            if (registry.all_of<LocalTransform>(mSelectedEntity) &&
                !registry.any_of<PointLightData>(mSelectedEntity))
            {
                ImGui::BeginDisabled(mLocked);
                RenderMeshProperties(registry);
                ImGui::EndDisabled();
            }

            if (registry.all_of<PhysicsComponent, PhysicsControlComponent>(mSelectedEntity))
                RenderBodyProperties(registry);

            ImGui::BeginDisabled(mLocked);

            if (registry.all_of<MeshMaterialData>(mSelectedEntity))
                RenderMaterialProperties(scene);

            else if (registry.all_of<DirectionalLightData>(mSelectedEntity))
                RenderDirectionalLightProperties(registry);

            else if (registry.all_of<PointLightData>(mSelectedEntity))
                RenderPointLightProperties(registry);

            else if (registry.all_of<CameraComponent>(mSelectedEntity))
                RenderCameraProperties(scene);

            ImGui::EndDisabled();
        }
        ImGui::End();
        ImGui::PopStyleColor();

        RenderGizmo(scene);
    }

    void EntityEditorPanel::SetCameraData(const CameraData &data)
    {
        mCameraData = data;
    }

    void EntityEditorPanel::SetSelectedEntity(entt::entity entity, const Scene &scene)
    {
        mSelectedEntity = entity;
        if (entity == entt::null)
            mOpen = false;

        if (const auto *local = scene.GetEntityRegistry().try_get<LocalTransform>(mSelectedEntity))
            mRotationDegrees = glm::degrees(glm::eulerAngles(local->rotation));
    }

    bool EntityEditorPanel::IsGizmoInUse() const
    {
        return mGizmoInUse;
    }

    void EntityEditorPanel::SetSimulationState(bool running)
    {
        mSimulationRunning = running;
    }

    void EntityEditorPanel::SetOnBodyTypeChange(OnBodyTypeChange &&callback)
    {
        mOnBodyTypeChange = std::move(callback);
    }

    void EntityEditorPanel::SetOnBodyScaleChange(OnBodyScaleChange &&callback)
    {
        mOnBodyScaleChange = std::move(callback);
    }

    void EntityEditorPanel::RenderGizmo(Scene &scene)
    {
        if (mLocked)
        {
            mGizmoInUse       = false;
            mGizmoWasDragging = false;
            return;
        }

        auto &registry = scene.GetEntityRegistry();

        if (!registry.all_of<LocalTransform>(mSelectedEntity))
            return;
        bool isMesh = registry.all_of<MeshComponent>(mSelectedEntity);
        auto operation = isMesh ? mCurrentGizmoOperation : ImGuizmo::TRANSLATE;

        auto modelMatrix{registry.get<WorldTransform>(mSelectedEntity).model};

        ImGuizmo::Manipulate(
            glm::value_ptr(mCameraData.view),
            glm::value_ptr(mCameraData.projection),
            operation,
            mCurrentGizmoMode,
            glm::value_ptr(modelMatrix)
        );

        bool isDragging = ImGuizmo::IsUsing();
        if (mGizmoWasDragging && !isDragging)
            if (operation == ImGuizmo::SCALE &&
                registry.all_of<PhysicsComponent>(mSelectedEntity))
                mOnBodyScaleChange(
                    registry.get<PhysicsComponent>(mSelectedEntity),
                    registry.get<LocalTransform>(mSelectedEntity).scale
                    );

        mGizmoWasDragging = isDragging;

        mGizmoInUse = isDragging || ImGuizmo::IsOver();
        if (isDragging)
        {
            scene.SetWorldTransform(mSelectedEntity, modelMatrix);
            auto local = registry.get<LocalTransform>(mSelectedEntity);
            if (mUniformScale && operation == ImGuizmo::SCALE)
            {
                const float avgScale = (local.scale.x + local.scale.y + local.scale.z) / 3.f;
                local.scale = glm::vec3(avgScale);
                scene.SetLocalTransform(mSelectedEntity, local);
            }

            mRotationDegrees = glm::degrees(glm::eulerAngles(local.rotation));
        }
    }

    void EntityEditorPanel::RenderHeader(const entt::registry &registry) const
    {
        const auto &[name]{registry.get<TagComponent>(mSelectedEntity)};

        std::string_view kind{"Entity"};
        if (registry.all_of<DirectionalLightData>(mSelectedEntity)) kind = "Directional light";
        else if (registry.all_of<PointLightData>(mSelectedEntity))  kind = "Point light";
        else if (registry.all_of<CameraComponent>(mSelectedEntity)) kind = "Camera";
        else if (registry.all_of<MeshComponent>(mSelectedEntity))   kind = "Mesh";

        ImGui::SetWindowFontScale(1.15f);
        ImGui::TextUnformatted(name.c_str());
        ImGui::SetWindowFontScale(1.f);
        ImGui::TextDisabled("%s", kind.data());
        ImGui::Dummy(ImVec2(0.f, 4.f));
    }

    void EntityEditorPanel::RenderMeshProperties(entt::registry &registry)
    {
        if (registry.all_of<MeshComponent>(mSelectedEntity))
            widgets::segmented("##gizmo_op", mCurrentGizmoOperation, kOperations, kOperationValues);
        
        widgets::segmented("##gizmo_mode", mCurrentGizmoMode, kOperationModeLabels, kOperationModes);

        auto &transform = registry.get<LocalTransform>(mSelectedEntity);
        if (!widgets::section("Transform"))
            return;

        if (!widgets::begin_properties("##transform"))
            return;

         widgets::property("Position");
         widgets::vec3_control("pos", transform.position);

         widgets::property("Rotation");
        if (widgets::vec3_control("rot", mRotationDegrees, 0.5f, 0.f, nullptr, "%.1f"))
            transform.rotation = glm::quat(glm::radians(mRotationDegrees));

         widgets::property("Scale");
        {
            const float lockWidth = ImGui::GetFrameHeight() + 6.f;
            const float fieldsWidth = ImGui::GetContentRegionAvail().x - lockWidth;

            const auto oldScale = transform.scale;
            bool editDone{};
            if (widgets::vec3_control("scale", transform.scale, 0.02f, 1.f, &editDone, "%.2f", fieldsWidth) &&
                mUniformScale)
            {
                float newValue = oldScale.x;
                if (transform.scale.x != oldScale.x)      newValue = transform.scale.x;
                else if (transform.scale.y != oldScale.y) newValue = transform.scale.y;
                else if (transform.scale.z != oldScale.z) newValue = transform.scale.z;
                transform.scale = glm::vec3(newValue);
            }

            ImGui::SameLine(0.f, 6.f);
            ImGui::Checkbox("##uniform", &mUniformScale);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Uniform scale");

            if (editDone && registry.all_of<PhysicsComponent>(mSelectedEntity))
                mOnBodyScaleChange(registry.get<PhysicsComponent>(mSelectedEntity), transform.scale);
        }

         widgets::end_properties();
    }

    void EntityEditorPanel::RenderBodyProperties(entt::registry &registry)
    {
        if (!widgets::section("Physics"))
            return;

        auto &physics = registry.get<PhysicsComponent>(mSelectedEntity);
        auto &control = registry.get<PhysicsControlComponent>(mSelectedEntity);

        if (!widgets::begin_properties("##physics"))
            return;

        ImGui::BeginDisabled(mSimulationRunning);

         widgets::property("Body type");
        int typeIndex = static_cast<int>(physics.type);
        if (ImGui::Combo("##body_type", &typeIndex, HierarchyPanel::s_BodyTypeOptions.data()))
        {
            physics.type = static_cast<PhysicsBodyType>(typeIndex);
            mOnBodyTypeChange(physics, physics.type);
        }

         widgets::property("Velocity");
         widgets::vec3_control("vel", control.velocity);

        ImGui::EndDisabled();

         widgets::property("Force");
         widgets::vec3_control("force", control.force);

         widgets::property("");
        ImGui::Checkbox("Apply force continuously", &control.applyForce);

         widgets::property("Impulse");
         widgets::vec3_control("impulse", control.impulse);

         widgets::property("");
        control.applyImpulse = ImGui::Button("Apply impulse", ImVec2(-FLT_MIN, 0.f));

         widgets::end_properties();
    }

    void EntityEditorPanel::RenderMaterialProperties(Scene &scene) const
    {
        if (!widgets::section("Material"))
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

         widgets::end_properties();

        if (changed)
            propagate_material_to_children(scene, mSelectedEntity, material);
    }

    void EntityEditorPanel::RenderDirectionalLightProperties(entt::registry &registry) const
    {
        auto &data = registry.get<DirectionalLightData>(mSelectedEntity);
        if (!widgets::section("Light"))
            return;

        if (!widgets::begin_properties("##sun"))
            return;

        float &enableValue = data.colorAndEnabled.w;
        bool enabled = enableValue > 0.5f;
         widgets::property("Enabled");
        if (ImGui::Checkbox("##enabled", &enabled))
            enableValue = enabled ? 1.f : 0.f;

         widgets::property("Color");
        ImGui::ColorEdit3("##color", glm::value_ptr(data.colorAndEnabled),
                          ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_PickerHueWheel);

         widgets::property("Intensity");
        ImGui::DragFloat("##intensity", &data.directionAndIntensity.w, 0.05f, 0.f, FLT_MAX, "%.2f");

         widgets::property("Direction");
        glm::vec3 dir{data.directionAndIntensity};
        if (widgets::vec3_control("dir", dir, 0.01f))
            data.directionAndIntensity = glm::vec4(glm::clamp(dir, glm::vec3(-1.f), glm::vec3(1.f)),
                                                   data.directionAndIntensity.w);

         widgets::end_properties();
    }

    void EntityEditorPanel::RenderPointLightProperties(entt::registry &registry) const
    {
        auto [light, local] = registry.get<PointLightData, LocalTransform>(mSelectedEntity);
        if (!widgets::section("Point light"))
            return;

        if (!widgets::begin_properties("##point_light"))
            return;

        float &enableValue = light.colorAndEnabled.w;
        bool enabled = enableValue > 0.5f;
         widgets::property("Enabled");
        if (ImGui::Checkbox("##enabled", &enabled))
            enableValue = enabled ? 1.f : 0.f;

         widgets::property("Position");
         widgets::vec3_control("pos", local.position);

         widgets::property("Color");
        if (ImGui::ColorEdit3("##color", glm::value_ptr(light.colorAndEnabled),
                              ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_PickerHueWheel))
            registry.get<GizmoComponent>(mSelectedEntity).color = {glm::vec3(light.colorAndEnabled), 1.f};

         widgets::property("Intensity");
        ImGui::DragFloat("##intensity", &light.positionAndIntensity.w, 0.1f, 0.f, FLT_MAX, "%.2f");

         widgets::property("Range");
        ImGui::DragFloat("##range", &light.range.x, 0.1f, 0.f, FLT_MAX, "%.2f");

         widgets::end_properties();
    }

    void EntityEditorPanel::RenderCameraProperties(Scene &scene) const
    {
        auto &registry{scene.GetEntityRegistry()};
        auto &camera = registry.get<CameraComponent>(mSelectedEntity);
        if (!widgets::section("Camera"))
            return;

        if (!widgets::begin_properties("##camera"))
            return;

         widgets::property("Exposure");
        ImGui::DragFloat("##exposure", &camera.exposure, 0.00001f, 0.f, FLT_MAX, "%.6f");

        if (!registry.all_of<BuiltinCamera>(mSelectedEntity))
        {
             widgets::property("Primary");
            if (ImGui::Checkbox("##primary", &camera.isPrimary) && camera.isPrimary)
                scene.SetPrimaryCamera(mSelectedEntity);
        }

         widgets::end_properties();
    }

    void EntityEditorPanel::propagate_material_to_children(Scene &scene, entt::entity entity,
                                                           const MeshMaterialData &material)
    {
        auto& registry = scene.GetEntityRegistry();
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