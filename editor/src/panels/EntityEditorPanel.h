#pragma once
#include "Panel.h"

namespace kailux
{
    class EntityEditorPanel : public Panel
    {
    public:
        EntityEditorPanel();
        EntityEditorPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor);

        void render(Scene &scene) override;

        void setSelectedEntity(entt::entity entity, const Scene &scene);

    private:
        entt::entity m_SelectedEntity;
        glm::vec3    m_RotationDegrees;
    };
}
