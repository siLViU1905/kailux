#pragma once
#include "Panel.h"
#include <ImGuizmo.h>

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
        entt::entity m_SelectedEntity;
        glm::vec3    m_RotationDegrees;

        ImGuizmo::OPERATION m_CurrentGizmoOperation;
        ImGuizmo::MODE      m_CurrentGizmoMode;

        bool                m_UniformScale;
        bool                m_GizmoInUse;
    };
}
