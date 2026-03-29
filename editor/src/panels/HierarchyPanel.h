#pragma once
#include "Panel.h"

namespace kailux
{
    class HierarchyPanel : public Panel
    {
    public:
        HierarchyPanel();
        HierarchyPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor);

        void render(Scene &scene) override;

        entt::entity getSelectedEntity() const;

    private:
        entt::entity m_SelectedEntity;
    };
}
