#pragma once
#include "Layer.h"
#include "core/panels/project_panel/ProjectPanel.h"
#include "core/panels/EntityEditorPanel.h"
#include "core/panels/HierarchyPanel.h"
#include "core/panels/MenuPanel.h"
#include "core/panels/ViewportPanel.h"

namespace kailux
{
    class EditorLayer
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(EditorLayer)

        static EditorLayer create(ImTextureID directoryTextureId, ImTextureID fileTextureId);

        void render(Scene& scene);

        auto&       getLayer()
        {
            return m_Layer;
        }
        const auto& getLayer() const
        {
            return m_Layer;
        }

        void update();

    private:
        static constexpr std::string_view s_HierarchyPanelName = "EntitiesHierarchy";
        static constexpr ImVec2           s_HierarchyPanelPosition = {0.8f, 0.f};
        static constexpr ImVec2           s_HierarchyPanelSize = {
                                              1.f - s_HierarchyPanelPosition.x,
                                              0.6f
                                          };

        static constexpr std::string_view s_EntityEditorName = "EntityEditor";
        static constexpr ImVec2           s_EntityEditorPosition = {s_HierarchyPanelPosition.x, s_HierarchyPanelPosition.y + s_HierarchyPanelSize.y};
        static constexpr ImVec2           s_EntityEditorSize = {
                                              1.f - s_EntityEditorPosition.x,
                                              1.f - s_EntityEditorPosition.y
                                          };

        static constexpr std::string_view s_ProjectPanelName = "ProjectPanel";
        static constexpr ImVec2           s_ProjectPanelPosition = {0.f, s_EntityEditorPosition.y};
        static constexpr ImVec2           s_ProjectPanelSize = {
                                                s_EntityEditorPosition.x,
                                              1.0f - s_EntityEditorPosition.y
                                          };

        static constexpr ImVec4   s_PanelsBackgroundColor = {0.15f, 0.15f, 0.15f, 1.f};

        void addPanels(ImTextureID directoryTextureId, ImTextureID fileTextureId);

        Layer<ViewportPanel, MenuPanel, HierarchyPanel, EntityEditorPanel, ProjectPanel> m_Layer;
    };
}
