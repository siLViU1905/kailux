#pragma once
#include "Panel.h"
#include <ImGuizmo.h>

#include "core/components/entt/PhysicsComponent.h"
#include "core/components/gpu/CameraData.h"

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

        using OnBodyTypeChange = std::move_only_function<void(PhysicsComponent, PhysicsBodyType)>;
        void SetOnBodyTypeChange(OnBodyTypeChange&& callback);

        using OnBodyScaleChange = std::move_only_function<void(PhysicsComponent, const glm::vec3&)>;
        void SetOnBodyScaleChange(OnBodyScaleChange&& callback);

    private:
        static constexpr std::array<std::string_view, 3> kOperations{"Move", "Rotate", "Scale"};
        static constexpr std::array                      kOperationValues{ImGuizmo::TRANSLATE, ImGuizmo::ROTATE, ImGuizmo::SCALE};

        static constexpr std::array<std::string_view, 2> kOperationModeLabels{"Local", "World"};
        static constexpr std::array                      kOperationModes{ImGuizmo::LOCAL, ImGuizmo::WORLD};

        void RenderGizmo(Scene& scene);
        void RenderHeader(const entt::registry &registry) const;
        void RenderMeshProperties(entt::registry &registry);
        void RenderBodyProperties(entt::registry &registry);
        void RenderMaterialProperties(Scene &scene) const;
        void RenderDirectionalLightProperties(entt::registry &registry) const;
        void RenderPointLightProperties(entt::registry &registry) const;
        void RenderCameraProperties(Scene &scene) const;

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

        OnBodyTypeChange  mOnBodyTypeChange;
        OnBodyScaleChange mOnBodyScaleChange;
    };
}
