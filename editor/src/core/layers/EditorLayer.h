#pragma once
#include "Layer.h"
#include "core/panels/project_panel/ProjectPanel.h"
#include "core/panels/EntityEditorPanel.h"
#include "core/panels/HierarchyPanel.h"
#include "core/panels/MenuPanel.h"
#include "core/panels/ViewportPanel.h"

namespace kailux
{
    class EditorLayer final : public Layer
    {
    public:
        EditorLayer(ImTextureID dirTex, ImTextureID fileTex);

        void render(Scene& scene) const;
        void update();

    private:
        static constexpr std::string_view s_HierarchyPanelName = "EntitiesHierarchy";
        static constexpr std::string_view s_EntityEditorName = "EntityEditor";
        static constexpr std::string_view s_ProjectPanelName = "ProjectPanel";

        static constexpr ImVec4   s_PanelsBackgroundColor = {0.15f, 0.15f, 0.15f, 1.f};

        static void render_dock_space();

        void addPanels(ImTextureID directoryTextureId, ImTextureID fileTextureId);
    };
}
