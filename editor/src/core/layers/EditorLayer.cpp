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

    EditorLayer EditorLayer::create()
    {
        EditorLayer layer;
        layer.addPanels();
        return layer;
    }

    void EditorLayer::render(Scene &scene)
    {
        m_Layer.render(scene);
    }

    void EditorLayer::update()
    {
        m_Layer.getPanel<AssetBrowserPanel>().useFullWidth(!m_Layer.getPanel<EntityEditorPanel>().isOpen());
    }

    void EditorLayer::addPanels()
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
        std::get<AssetBrowserPanel>(panels) = {
            s_AssetBrowserName,
            s_AssetBrowserPosition,
            s_AssetBrowserSize,
            s_PanelsBackgroundColor
        };

        hierarchyPanel.setOnEntitySelected([&entityEditorPanel](entt::entity entity, const Scene &scene)
        {
            entityEditorPanel.setSelectedEntity(entity, scene);
            entityEditorPanel.open();
        });
    }
}
