#include "EditorLayer.h"

#include "../panels/EntityEditorPanel.h"
#include "../panels/HierarchyPanel.h"
#include "../panels/MenuPanel.h"

namespace kailux
{
    EditorLayer::EditorLayer(ImTextureID dirTex, ImTextureID fileTex)
    {
        addPanels(dirTex, fileTex);
    }

    void EditorLayer::render(Scene &scene) const
    {
        render_dock_space();

        renderPanels(scene);
    }

    void EditorLayer::update()
    {
        getPanel<ProjectPanel>().useFullWidth(!getPanel<EntityEditorPanel>().isOpen());

        bool isSimulationRunning = getPanel<ViewportPanel>().getSimulationState() != SimulationState::Paused;
        getPanel<EntityEditorPanel>().setSimulationState(isSimulationRunning);
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

    void EditorLayer::addPanels(ImTextureID directoryTextureId, ImTextureID fileTextureId)
    {
        emplacePanel<ViewportPanel>();
        auto& menuPanel = emplacePanel<MenuPanel>();
        auto &hierarchyPanel = emplacePanel<HierarchyPanel>() = {
                                   kHierarchyPanelName,
                                   kPanelsBackgroundColor
                               };
        auto &entityEditorPanel = emplacePanel<EntityEditorPanel>() = {
                                      kEntityEditorName,
                                      kPanelsBackgroundColor
                                  };
        auto &projectPanel = emplacePanel<ProjectPanel>() = {
                                 kProjectPanelName,
                                 kPanelsBackgroundColor
                             };

        hierarchyPanel.setOnEntitySelected([&entityEditorPanel](entt::entity entity, const Scene &scene)
        {
            entityEditorPanel.open();
            entityEditorPanel.setSelectedEntity(entity, scene);
        });

        projectPanel.getAssetBrowser().setDirectoryTextureId(directoryTextureId);
        projectPanel.getAssetBrowser().setFileTextureId(fileTextureId);

        menuPanel.setOnViewMenu([&hierarchyPanel, &entityEditorPanel, &projectPanel]()
        {
            if (ImGui::MenuItem("Entities Hierarchy", nullptr, hierarchyPanel.isOpen()))
                hierarchyPanel.toggle();
            if (ImGui::MenuItem("Entity Editor", nullptr, entityEditorPanel.isOpen()))
                entityEditorPanel.toggle();
            if (ImGui::MenuItem("Project", nullptr, projectPanel.isOpen()))
                projectPanel.toggle();
        });
    }
}
