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
        static constexpr std::string_view s_BodyTypeOptions = {
            "Static\0"
            "Kinematic\0"
            "Dynamic\0",
            sizeof("Static\0Dynamic\0Kinematic\0")
        };

        void renderGizmo(Scene& scene);

        static void propagate_material_to_children(Scene &scene, entt::entity entity, const MeshMaterialData& material);

        entt::entity m_SelectedEntity;
        glm::vec3    m_RotationDegrees;

        ImGuizmo::OPERATION m_CurrentGizmoOperation;
        ImGuizmo::MODE      m_CurrentGizmoMode;

        bool                m_UniformScale;
        bool                m_GizmoInUse;
        bool                m_GizmoWasDragging;
        bool                m_SimulationRunning;

        OnBodyTypeChange  m_OnBodyTypeChange;
        OnBodyScaleChange m_OnBodyScaleChange;
    };
}
