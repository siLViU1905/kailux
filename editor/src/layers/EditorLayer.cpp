#include "EditorLayer.h"

#include "../panels/EntityEditorPanel.h"
#include "../panels/HierarchyPanel.h"

namespace kailux
{
    EditorLayer EditorLayer::create()
    {
        EditorLayer layer;
        layer.addPanels();
        return layer;
    }

    void EditorLayer::addPanels()
    {
        m_Panels.resize(s_PanelsCount);
        m_Panels[s_HierarchyPanelIndex] = create_scoped<HierarchyPanel>(
                s_HierarchyPanelName,
                s_HierarchyPanelPosition,
                s_HierarchyPanelSize,
                s_PanelsBackgroundColor
            );
        m_Panels[s_EntityEditorIndex] = create_scoped<EntityEditorPanel>(
                s_EntityEditorName,
                s_EntityEditorPosition,
                s_EntityEditorSize,
                s_PanelsBackgroundColor
            );

        auto& hierarchyPanel = static_cast<HierarchyPanel&>(*m_Panels[s_HierarchyPanelIndex]);
        auto& entityEditorPanel = static_cast<EntityEditorPanel&>(*m_Panels[s_EntityEditorIndex]);

        hierarchyPanel.setOnEntitySelected([&entityEditorPanel](entt::entity entity, const Scene& scene)
        {
            entityEditorPanel.setSelectedEntity(entity, scene);
        });
    }
}
