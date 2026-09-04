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

        void Render(Scene& scene) const;
        void Update();

    private:
        static constexpr std::string_view kHierarchyPanelName{"EntitiesHierarchy"};
        static constexpr std::string_view kEntityEditorName{"EntityEditor"};
        static constexpr std::string_view kProjectPanelName{"ProjectPanel"};
        static constexpr std::string_view kViewportPanelName{"SceneViewport"};
        static constexpr std::string_view kSimulationPanelName{"Simulation"};


        static constexpr ImVec4 kPanelsBackgroundColor{0.122f, 0.122f, 0.122f, 1.f};

        static void render_dock_space();

        void AddPanels(ImTextureID directoryTextureId, ImTextureID fileTextureId);

        bool mSimulationWasRunning{};
    };
}
