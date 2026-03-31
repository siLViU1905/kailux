#pragma once
#include "layers/Layer.h"

namespace kailux
{
    class Editor
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Editor)

        static Editor create();

        void render(Scene& scene) const;

    private:
        void createLayers();

        Scoped<Layer> m_ActiveLayer;
    };
}
