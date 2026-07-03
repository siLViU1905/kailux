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

        void render(Scene &scene) override;

        void setSelectedEntity(entt::entity entity, const Scene &scene);

        bool isGizmoInUse() const;

        void setSimulationState(bool running);

        using OnBodyTypeChange = std::move_only_function<void(PhysicsComponent, PhysicsBodyType)>;
        void setOnBodyTypeChange(OnBodyTypeChange&& callback);

        using OnBodyScaleChange = std::move_only_function<void(PhysicsComponent, const glm::vec3&)>;
        void setOnBodyScaleChange(OnBodyScaleChange&& callback);

    private:
        void renderGizmo(Scene& scene);

        static void propagate_material_to_children(Scene &scene, entt::entity entity, const MeshMaterialData& material);

        entt::entity mSelectedEntity;
        glm::vec3    mRotationDegrees;

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
