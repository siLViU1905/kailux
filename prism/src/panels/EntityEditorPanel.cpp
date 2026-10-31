#include "EntityEditorPanel.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "HierarchyPanel.h"
#include "components/entt/BuiltinCamera.h"
#include "components/entt/CameraComponent.h"
#include "components/entt/HierarchyComponent.h"
#include "components/entt/LocalTransform.h"
#include "components/entt/PhysicsControlComponent.h"
#include "components/entt/TagComponent.h"
#include "components/entt/WorldTransform.h"
#include "components/gpu/CameraData.h"
#include "../UIWidgets.h"
#include "audio/AudioLoader.h"
#include "imgui_backend/misc/imgui_stdlib.h"
#include "components/entt/AudioSourceComponent.h"
#include "project_panel/AssetBrowser.h"

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

            if (registry.all_of<AudioSourceComponent>(mSelectedEntity))
                RenderAudioSourceProperties(scene);

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

    void EntityEditorPanel::SetOnTransformChange(OnTransformChange &&callback)
    {
        mOnTransformChange = std::move(callback);
    }

    void EntityEditorPanel::SetOnBodyTypeChange(OnBodyTypeChange &&callback)
    {
        mOnBodyTypeChange = std::move(callback);
    }

    void EntityEditorPanel::SetOnBodyMassChange(OnBodyMassChange &&callback)
    {
        mOnBodyMassChange = std::move(callback);
    }

    void EntityEditorPanel::SetOnRemoveAudioSource(OnRemoveAudioSource &&callback)
    {
        mOnRemoveAudioSource = std::move(callback);
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
                mOnTransformChange(mSelectedEntity, registry.get<LocalTransform>(mSelectedEntity), true);

        mGizmoWasDragging = isDragging;

        mGizmoInUse = isDragging || ImGuizmo::IsOver();
        if (isDragging)
        {
            auto local{Transform::from_matrix(glm::inverse(scene.GetParentWorldMatrix(mSelectedEntity)) * modelMatrix)};
            if (mUniformScale && operation == ImGuizmo::SCALE)
            {
                const float avgScale = (local.scale.x + local.scale.y + local.scale.z) / 3.f;
                local.scale = glm::vec3(avgScale);
            }

            mRotationDegrees = glm::degrees(glm::eulerAngles(local.rotation));
            mOnTransformChange(mSelectedEntity, local, false);
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

    void EntityEditorPanel::RenderMeshProperties(const entt::registry &registry)
    {
        if (registry.all_of<MeshComponent>(mSelectedEntity))
            widgets::segmented("##gizmo_op", mCurrentGizmoOperation, kOperations, kOperationValues);
        
        widgets::segmented("##gizmo_mode", mCurrentGizmoMode, kOperationModeLabels, kOperationModes);

        Transform transform{registry.get<LocalTransform>(mSelectedEntity)};
        bool      changed{};
        if (!widgets::section("Transform"))
            return;

        if (!widgets::begin_properties("##transform"))
            return;

         widgets::property("Position");
        changed |= widgets::vec3_control("pos", transform.position);

         widgets::property("Rotation");
        if (widgets::vec3_control("rot", mRotationDegrees, 5.f, 0.f, nullptr, "%.1f"))
        {
            transform.rotation = glm::quat(glm::radians(mRotationDegrees));
            changed = true;
        }

         widgets::property("Scale");
        {
            const float lockWidth = ImGui::GetFrameHeight() + 6.f;
            const float fieldsWidth = ImGui::GetContentRegionAvail().x - lockWidth;

            const auto oldScale = transform.scale;
            bool editDone{};
            if (widgets::vec3_control("scale", transform.scale, 0.02f, 1.f, &editDone, "%.3f", fieldsWidth))
            {
                changed = true;
                if (mUniformScale)
                {
                    float newValue = oldScale.x;
                    if (transform.scale.x != oldScale.x)      newValue = transform.scale.x;
                    else if (transform.scale.y != oldScale.y) newValue = transform.scale.y;
                    else if (transform.scale.z != oldScale.z) newValue = transform.scale.z;
                    transform.scale = glm::vec3(newValue);
                }
            }

            ImGui::SameLine(0.f, 6.f);
            ImGui::Checkbox("##uniform", &mUniformScale);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Uniform scale");

            const bool syncBodyScale{editDone && registry.all_of<PhysicsComponent>(mSelectedEntity)};
            if (changed || syncBodyScale)
                mOnTransformChange(mSelectedEntity, transform, syncBodyScale);
        }

         widgets::end_properties();
    }

    void EntityEditorPanel::RenderBodyProperties(entt::registry &registry)
    {
        if (!widgets::section("Physics"))
            return;

        const auto &physics = registry.get<PhysicsComponent>(mSelectedEntity);
        auto &control = registry.get<PhysicsControlComponent>(mSelectedEntity);

        if (!widgets::begin_properties("##physics"))
            return;

        ImGui::BeginDisabled(mSimulationRunning);

         widgets::property("Body type");
        const bool locked{!physics.canBecomeDynamic};
        ImGui::BeginDisabled(locked);
        int typeIndex = static_cast<int>(physics.type);
        if (ImGui::Combo("##body_type", &typeIndex, HierarchyPanel::s_BodyTypeOptions.data()))
            mOnBodyTypeChange(mSelectedEntity, static_cast<PhysicsBodyType>(typeIndex));
        ImGui::EndDisabled();
        if (locked)
            ImGui::SetItemTooltip("Created as a static mesh; re-add physics with \"can become dynamic\" to change type");

        widgets::property("Mass");
        ImGui::BeginDisabled(!physics.canBecomeDynamic);
        ImGui::SetNextItemWidth(-std::numeric_limits<float>::min());
        float mass{physics.mass};
        if (ImGui::DragFloat("##mass", &mass, 0.5f, 0.f, 1.0e6f,
                             mass > 0.f ? "%.2f kg" : "auto", ImGuiSliderFlags_AlwaysClamp))
            mOnBodyMassChange(mSelectedEntity, mass, false);
        if (ImGui::IsItemDeactivatedAfterEdit())
            mOnBodyMassChange(mSelectedEntity, mass, true);
        ImGui::EndDisabled();
        ImGui::SetItemTooltip("0 = auto, computed from volume (1000 kg/m^3)");

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

    void EntityEditorPanel::RenderAudioSourceProperties(Scene &scene)
    {
        if (!widgets::section("Audio source"))
            return;

        auto &audio = scene.GetEntityRegistry().get<AudioSourceComponent>(mSelectedEntity);
        auto &settings = audio.settings;

        if (!widgets::begin_properties("##audio_source"))
            return;

        widgets::property("Clip");
        ImGui::InputTextWithHint("##clip", "drop a .wav, .ogg, .mp3 or .flac", &audio.clip);
        if (ImGui::BeginDragDropTarget())
        {
            if (const auto *payload = ImGui::AcceptDragDropPayload(AssetBrowser::s_DragDropPayloadType.data()))
            {
                const std::filesystem::path path{static_cast<const char *>(payload->Data)};
                if (audio_loader::is_supported(path))
                    audio.clip = path.generic_string();
            }
            ImGui::EndDragDropTarget();
        }

        widgets::property("Volume");
        ImGui::SliderFloat("##gain", &settings.gain, 0.f, 2.f, "%.2f");

        widgets::property("Pitch");
        ImGui::SliderFloat("##pitch", &settings.pitch, 0.25f, 4.f, "%.2f");

        widgets::property("Loop");
        ImGui::Checkbox("##loop", &settings.loop);

        widgets::property("Play on start");
        ImGui::Checkbox("##play_on_start", &settings.playOnStart);

        widgets::property("Spatial (3D)");
        ImGui::Checkbox("##spatial", &settings.spatial);
        ImGui::SetItemTooltip("Positioned at the entity and heard from the primary camera; the clip is mixed down to mono");
        ImGui::BeginDisabled(!settings.spatial);

        widgets::property("Ref. distance");
        ImGui::DragFloat("##reference_distance", &settings.referenceDistance, 0.1f, 0.f, FLT_MAX, "%.1f m");
        ImGui::SetItemTooltip("Distance at which the source plays at full volume");

        widgets::property("Max distance");
        ImGui::DragFloat("##max_distance", &settings.maxDistance, 0.5f, 0.f, FLT_MAX, "%.1f m");
        ImGui::SetItemTooltip("Beyond this distance the volume stops dropping");

        widgets::property("Rolloff");
        ImGui::DragFloat("##rolloff", &settings.rolloff, 0.05f, 0.f, 10.f, "%.2f");
        ImGui::EndDisabled();

        widgets::property("");
        const bool remove{ImGui::Button("Remove audio source", ImVec2(-FLT_MIN, 0.f))};
         widgets::end_properties();
        if (remove)
            mOnRemoveAudioSource(mSelectedEntity);
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
