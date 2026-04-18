#include "Editor.h"

#include "layers/EditorLayer.h"

namespace kailux
{
    Editor::Editor()
    {
        createLayers();
    }

    Editor::Editor(Editor &&other) noexcept : m_ActiveLayer(std::move(other.m_ActiveLayer))
    {
    }

    Editor & Editor::operator=(Editor &&other) noexcept
    {
        if (this != &other)
        {
            m_ActiveLayer = std::move(other.m_ActiveLayer);
        }
        return *this;
    }

    Editor Editor::create()
    {
        Editor editor;
        editor.createLayers();
        return editor;
    }

    void Editor::render(Scene &scene)
    {
        std::visit([&scene](auto& layer)
        {
            layer.render(scene);
        }, *m_ActiveLayer);
    }

    void Editor::createLayers()
    {
        m_ActiveLayer = create_scoped<LayerTypes>();
        m_ActiveLayer->emplace<EditorLayer>(EditorLayer::create());
    }
}
