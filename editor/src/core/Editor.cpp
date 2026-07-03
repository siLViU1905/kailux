#include "Editor.h"

#include "layers/EditorLayer.h"

namespace kailux
{
    Editor::Editor()
    {
    }

    Editor::Editor(Editor &&other) noexcept : mActiveLayer(std::move(other.mActiveLayer))
    {
    }

    Editor & Editor::operator=(Editor &&other) noexcept
    {
        if (this != &other)
        {
            mActiveLayer = std::move(other.mActiveLayer);
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
        }, *mActiveLayer);
    }

    void Editor::update()
    {
        std::visit([](auto& layer)
        {
            layer.update();
        }, *mActiveLayer);
    }

    void Editor::createLayers(ImTextureID directoryTextureId, ImTextureID fileTextureId)
    {
        mActiveLayer = create_scoped<LayerTypes>();
        mActiveLayer->emplace<EditorLayer>(EditorLayer::create(directoryTextureId, fileTextureId));
    }
}
