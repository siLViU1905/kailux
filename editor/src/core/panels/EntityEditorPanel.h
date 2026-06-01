#pragma once
#include "Panel.h"
#include <ImGuizmo.h>

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

    private:
        void renderGizmo(Scene& scene);

        static void propagate_material_to_children(Scene &scene, entt::entity entity, const MeshMaterialData& material);

        entt::entity m_SelectedEntity;
        glm::vec3    m_RotationDegrees;

        ImGuizmo::OPERATION m_CurrentGizmoOperation;
        ImGuizmo::MODE      m_CurrentGizmoMode;

        bool                m_UniformScale;
        bool                m_GizmoInUse;
    };
}
