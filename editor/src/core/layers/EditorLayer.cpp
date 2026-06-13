#include "EditorLayer.h"

#include "../panels/EntityEditorPanel.h"
#include "../panels/HierarchyPanel.h"
#include "../panels/MenuPanel.h"

namespace kailux
{
    EditorLayer::EditorLayer()
    {
    }

    EditorLayer::EditorLayer(EditorLayer &&other) noexcept : m_Layer(std::move(other.m_Layer))
    {
    }

    EditorLayer &EditorLayer::operator=(EditorLayer &&other) noexcept
    {
        if (this != &other)
        {
            m_Layer = std::move(other.m_Layer);
        }
        return *this;
    }

    EditorLayer EditorLayer::create(ImTextureID directoryTextureId, ImTextureID fileTextureId)
    {
        EditorLayer layer;
        layer.addPanels(directoryTextureId, fileTextureId);
        return layer;
    }

    void EditorLayer::render(Scene &scene)
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

        m_Layer.render(scene);
    }

    void EditorLayer::update()
    {
        m_Layer.getPanel<ProjectPanel>().useFullWidth(!m_Layer.getPanel<EntityEditorPanel>().isOpen());

        bool isSimulationRunning = m_Layer.getPanel<ViewportPanel>().getSimulationState() != SimulationState::Paused;
        m_Layer.getPanel<EntityEditorPanel>().setSimulationState(isSimulationRunning);
    }

    void EditorLayer::addPanels(ImTextureID directoryTextureId, ImTextureID fileTextureId)
    {
        auto &panels = m_Layer.getPanels();
        std::get<ViewportPanel>(panels) = {};
        std::get<MenuPanel>(panels) = {};
        auto &hierarchyPanel = std::get<HierarchyPanel>(panels) = {
                                   s_HierarchyPanelName,
                                   s_PanelsBackgroundColor
                               };
        auto &entityEditorPanel = std::get<EntityEditorPanel>(panels) = {
                                      s_EntityEditorName,
                                      s_PanelsBackgroundColor
                                  };
        auto &projectPanel = std::get<ProjectPanel>(panels) = {
                                 s_ProjectPanelName,
                                 s_PanelsBackgroundColor
                             };

        hierarchyPanel.setOnEntitySelected([&entityEditorPanel](entt::entity entity, const Scene &scene)
        {
            entityEditorPanel.open();
            entityEditorPanel.setSelectedEntity(entity, scene);
        });

        projectPanel.getAssetBrowser().setDirectoryTextureId(directoryTextureId);
        projectPanel.getAssetBrowser().setFileTextureId(fileTextureId);
    }
}
