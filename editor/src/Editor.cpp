#include "Editor.h"

#include "layers/EditorLayer.h"

namespace kailux
{
    Editor::Editor()
    {
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

    void Editor::render(Scene &scene) const
    {
        m_ActiveLayer->render(scene);
    }

    void Editor::createLayers()
    {
        m_ActiveLayer = std::make_unique<EditorLayer>(EditorLayer::create());
    }
}
