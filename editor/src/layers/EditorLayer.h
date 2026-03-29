#pragma once
#include "Layer.h"

namespace kailux
{
    class EditorLayer : public Layer
    {
    public:
        static EditorLayer create();

    private:
        static constexpr std::string_view s_HierarchyPanelName = "EntitiesHierarchy";
        static constexpr ImVec2 s_HierarchyPanelPosition = {0.8f, 0.f};
        static constexpr ImVec2 s_HierarchyPanelSize = {
            1.f - s_HierarchyPanelPosition.x,
            0.6f
        };
        static constexpr ImVec4 s_PanelsBackgroundColor = {0.15f, 0.15f, 0.15f, 1.f};

        void addHierarchyPanel();
    };
}
