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
        m_Layer.render(scene);
    }

    void EditorLayer::update()
    {
        m_Layer.getPanel<ProjectPanel>().useFullWidth(!m_Layer.getPanel<EntityEditorPanel>().isOpen());
    }

    void EditorLayer::addPanels(ImTextureID directoryTextureId, ImTextureID fileTextureId)
    {
        auto &panels = m_Layer.getPanels();
        std::get<MenuPanel>(panels) = {};
        auto &hierarchyPanel = std::get<HierarchyPanel>(panels) = {
                                   s_HierarchyPanelName,
                                   s_HierarchyPanelPosition,
                                   s_HierarchyPanelSize,
                                   s_PanelsBackgroundColor
                               };
        auto &entityEditorPanel = std::get<EntityEditorPanel>(panels) = {
                                      s_EntityEditorName,
                                      s_EntityEditorPosition,
                                      s_EntityEditorSize,
                                      s_PanelsBackgroundColor
                                  };
        auto& projectPanel = std::get<ProjectPanel>(panels) = {
            s_ProjectPanelName,
            s_ProjectPanelPosition,
            s_ProjectPanelSize,
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
