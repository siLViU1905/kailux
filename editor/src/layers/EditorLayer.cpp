#include "EditorLayer.h"

#include "../panels/HierarchyPanel.h"

namespace kailux
{
    EditorLayer EditorLayer::create()
    {
        EditorLayer layer;
        layer.addHierarchyPanel();
        return layer;
    }

    void EditorLayer::addHierarchyPanel()
    {
        m_Panels.push_back(create_scoped<HierarchyPanel>(
                s_HierarchyPanelName,
                s_HierarchyPanelPosition,
                s_HierarchyPanelSize,
                s_PanelsBackgroundColor
            )
        );
    }
}
