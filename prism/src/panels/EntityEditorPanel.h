#pragma once
#include "Panel.h"
#include <ImGuizmo.h>

#include "components/entt/PhysicsComponent.h"
#include "components/gpu/CameraData.h"

namespace kailux
{
    class EntityEditorPanel : public Panel
    {
    public:
        EntityEditorPanel();
        EntityEditorPanel(std::string_view name, ImVec4 backgroundColor);

        void Render(Scene &scene) override;

        void SetCameraData(const CameraData& data);

        void SetSelectedEntity(entt::entity entity, const Scene &scene);

        bool IsGizmoInUse() const;

        void SetSimulationState(bool running);

        using OnTransformChange = std::move_only_function<void(entt::entity, const Transform&, bool syncBodyScale)>;
        void  SetOnTransformChange(OnTransformChange&& callback);

        using OnBodyTypeChange = std::move_only_function<void(entt::entity, PhysicsBodyType)>;
        void  SetOnBodyTypeChange(OnBodyTypeChange&& callback);

        using OnBodyMassChange = std::move_only_function<void(entt::entity, float mass, bool commit)>;
        void  SetOnBodyMassChange(OnBodyMassChange&& callback);

        using OnRemoveAudioSource = std::move_only_function<void(entt::entity)>;
        void  SetOnRemoveAudioSource(OnRemoveAudioSource&& callback);

    private:
        static constexpr std::array<std::string_view, 3> kOperations{"Move", "Rotate", "Scale"};
        static constexpr std::array                      kOperationValues{ImGuizmo::TRANSLATE, ImGuizmo::ROTATE, ImGuizmo::SCALE};

        static constexpr std::array<std::string_view, 2> kOperationModeLabels{"Local", "World"};
        static constexpr std::array                      kOperationModes{ImGuizmo::LOCAL, ImGuizmo::WORLD};

        void RenderGizmo(Scene& scene);
        void RenderHeader(const entt::registry &registry) const;
        void RenderMeshProperties(const entt::registry &registry);
        void RenderBodyProperties(entt::registry &registry);
        void RenderMaterialProperties(Scene &scene) const;
        void RenderDirectionalLightProperties(entt::registry &registry) const;
        void RenderPointLightProperties(entt::registry &registry) const;
        void RenderCameraProperties(Scene &scene) const;
        void RenderAudioSourceProperties(Scene &scene);

        static void propagate_material_to_children(Scene &scene, entt::entity entity, const MeshMaterialData& material);

        entt::entity mSelectedEntity;
        glm::vec3    mRotationDegrees;

        CameraData   mCameraData;

        ImGuizmo::OPERATION mCurrentGizmoOperation;
        ImGuizmo::MODE      mCurrentGizmoMode;

        bool                mUniformScale;
        bool                mGizmoInUse;
        bool                mGizmoWasDragging;
        bool                mSimulationRunning;

        OnTransformChange   mOnTransformChange;
        OnBodyTypeChange    mOnBodyTypeChange;
        OnBodyMassChange    mOnBodyMassChange;
        OnRemoveAudioSource mOnRemoveAudioSource;
    };
}
