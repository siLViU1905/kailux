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

        using OnEntitySelected = std::move_only_function<void(entt::entity, const Scene&)>;
        void  setOnEntitySelected(OnEntitySelected&& callback);

    private:
        OnEntitySelected m_OnEntitySelected;
        entt::entity     m_SelectedEntity;
    };
}
