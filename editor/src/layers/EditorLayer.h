#pragma once
#include "Layer.h"

namespace kailux
{
    class EditorLayer : public Layer
    {
    public:
        static EditorLayer create();

    private:
        static constexpr uint32_t         s_HierarchyPanelIndex = 0;
        static constexpr std::string_view s_HierarchyPanelName = "EntitiesHierarchy";
        static constexpr ImVec2           s_HierarchyPanelPosition = {0.8f, 0.f};
        static constexpr ImVec2           s_HierarchyPanelSize = {
                                              1.f - s_HierarchyPanelPosition.x,
                                              0.6f
                                          };

        static constexpr uint32_t         s_EntityEditorIndex = 1;
        static constexpr std::string_view s_EntityEditorName = "EntityEditor";
        static constexpr ImVec2           s_EntityEditorPosition = {s_HierarchyPanelPosition.x, s_HierarchyPanelPosition.y + s_HierarchyPanelSize.y};
        static constexpr ImVec2           s_EntityEditorSize = {
                                              1.f - s_EntityEditorPosition.x,
                                              1.f - s_EntityEditorPosition.y
                                          };

        static constexpr uint32_t s_PanelsCount = 2;
        static constexpr ImVec4   s_PanelsBackgroundColor = {0.15f, 0.15f, 0.15f, 1.f};

        void addPanels();
    };
}
