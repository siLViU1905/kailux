#include "EditorLayer.h"

#include "../panels/EntityEditorPanel.h"
#include "../panels/HierarchyPanel.h"
#include "../panels/MenuPanel.h"
#include "core/panels/SimulationPanel.h"

namespace kailux
{
    EditorLayer::EditorLayer(ImTextureID dirTex, ImTextureID fileTex)
    {
        AddPanels(dirTex, fileTex);
    }

    void EditorLayer::Render(Scene &scene) const
    {
        render_dock_space();

        RenderPanels(scene);
    }

    void EditorLayer::Update()
    {
        GetPanel<ProjectPanel>().UseFullWidth(!GetPanel<EntityEditorPanel>().IsOpen());

        auto& viewport = GetPanel<ViewportPanel>();
        auto& simulation = GetPanel<SimulationPanel>();

        const bool isSimulationRunning = viewport.GetSimulationState() != SimulationState::Paused;
        GetPanel<EntityEditorPanel>().SetSimulationState(isSimulationRunning);

        if (!isSimulationRunning)
            simulation.Close();
        else if (!mSimulationWasRunning)
            simulation.Open();
        else if (!simulation.IsOpen())
            viewport.RequestSimulationState(SimulationState::Paused);

        mSimulationWasRunning = isSimulationRunning;
    }

    void EditorLayer::render_dock_space()
    {
        const auto *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
                                                  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                                  ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavInputs;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        ImGui::Begin("KailuxDockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        auto dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.f, 0.f));

        ImGui::End();
    }

    void EditorLayer::AddPanels(ImTextureID directoryTextureId, ImTextureID fileTextureId)
    {
        auto &viewportPanel = EmplacePanel<ViewportPanel>(kViewportPanelName) ;
        auto& menuPanel = EmplacePanel<MenuPanel>();
        auto &hierarchyPanel = EmplacePanel<HierarchyPanel>(
            kHierarchyPanelName,
            kPanelsBackgroundColor);
        auto &entityEditorPanel = EmplacePanel<EntityEditorPanel>(kEntityEditorName,
                                                                  kPanelsBackgroundColor);
        auto &projectPanel = EmplacePanel<ProjectPanel>(kProjectPanelName,
                                                        kPanelsBackgroundColor);
        auto &simulationPanel = EmplacePanel<SimulationPanel>(kSimulationPanelName);
        simulationPanel.Close();

        hierarchyPanel.SetOnEntitySelected([&entityEditorPanel](entt::entity entity, const Scene &scene)
        {
            entityEditorPanel.Open();
            entityEditorPanel.SetSelectedEntity(entity, scene);
        });

        projectPanel.GetAssetBrowser().SetDirectoryTextureId(directoryTextureId);
        projectPanel.GetAssetBrowser().SetFileTextureId(fileTextureId);

        menuPanel.SetOnViewMenu([&hierarchyPanel, &entityEditorPanel, &projectPanel]()
        {
            if (ImGui::MenuItem("Entities Hierarchy", nullptr, hierarchyPanel.IsOpen()))
                hierarchyPanel.Toggle();
            if (ImGui::MenuItem("Entity Editor", nullptr, entityEditorPanel.IsOpen()))
                entityEditorPanel.Toggle();
            if (ImGui::MenuItem("Project", nullptr, projectPanel.IsOpen()))
                projectPanel.Toggle();
        });
    }
}
