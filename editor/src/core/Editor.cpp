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

    Editor Editor::create(ImTextureID directoryTextureId, ImTextureID fileTextureId)
    {
        Editor editor;
        editor.createLayers(directoryTextureId, fileTextureId);
        return editor;
    }

    void Editor::render(Scene &scene)
    {
        std::visit([&scene](auto& layer)
        {
            layer.render(scene);
        }, *m_ActiveLayer);
    }

    void Editor::update()
    {
        std::visit([](auto& layer)
        {
            layer.update();
        }, *m_ActiveLayer);
    }

    void Editor::createLayers(ImTextureID directoryTextureId, ImTextureID fileTextureId)
    {
        m_ActiveLayer = create_scoped<LayerTypes>();
        m_ActiveLayer->emplace<EditorLayer>(EditorLayer::create(directoryTextureId, fileTextureId));
    }
}
