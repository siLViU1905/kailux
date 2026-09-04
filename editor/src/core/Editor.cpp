#include "Editor.h"

#include "layers/EditorLayer.h"

namespace kailux
{
    Editor::Editor() = default;

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
        editor.CreateLayers(directoryTextureId, fileTextureId);
        return editor;
    }

    void Editor::Render(Scene &scene) const
    {
        std::visit([&scene](auto& layer)
        {
            layer.Render(scene);
        }, *mActiveLayer);
    }

    void Editor::OnEvent(const Event &event)
    {
        if (auto* editorLayer{std::get_if<EditorLayer>(mActiveLayer.get())})
        {
            if (const auto* keyReleased{std::get_if<KeyReleased>(&event)})
                switch (keyReleased->key)
                {
                    case Key::Delete:
                        editorLayer->GetPanel<HierarchyPanel>().DeleteSelectedEntity();
                        break;
                    default:
                        break;
                }
        }
    }

    void Editor::Update() const
    {
        std::visit([](auto& layer)
        {
            layer.Update();
        }, *mActiveLayer);
    }

    void Editor::CreateLayers(ImTextureID directoryTextureId, ImTextureID fileTextureId)
    {
        mActiveLayer = create_scoped<LayerTypes>(EditorLayer{directoryTextureId, fileTextureId});
    }
}
