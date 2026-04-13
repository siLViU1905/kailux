#pragma once
#include "layers/EditorLayer.h"

namespace kailux
{
    class Editor
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Editor)

        static Editor create();

        const EditorLayer& getEditorLayer() const;
        EditorLayer&       getEditorLayer();

        void render(Scene& scene) const;

    private:
        void createLayers();

        Shared<Layer> m_ActiveLayer;
        Shared<EditorLayer> m_EditorLayer;
    };
}
