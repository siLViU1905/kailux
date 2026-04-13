#include "Editor.h"

#include "layers/EditorLayer.h"

namespace kailux
{
    Editor::Editor()
    {
    }

    Editor::Editor(Editor &&other) noexcept : m_ActiveLayer(std::move(other.m_ActiveLayer)),
    m_EditorLayer(std::move(other.m_EditorLayer))
    {
    }

    Editor & Editor::operator=(Editor &&other) noexcept
    {
        if (this != &other)
        {
            m_ActiveLayer = std::move(other.m_ActiveLayer);
            m_EditorLayer = std::move(other.m_EditorLayer);
        }
        return *this;
    }

    Editor Editor::create()
    {
        Editor editor;
        editor.createLayers();
        return editor;
    }

    const EditorLayer & Editor::getEditorLayer() const
    {
        return *m_EditorLayer;
    }

    EditorLayer& Editor::getEditorLayer()
    {
        return *m_EditorLayer;
    }

    void Editor::render(Scene &scene) const
    {
        m_ActiveLayer->render(scene);
    }

    void Editor::createLayers()
    {
        m_EditorLayer = create_scoped<EditorLayer>(EditorLayer::create());
        m_ActiveLayer = m_EditorLayer;
    }
}
