#pragma once
#include "layers/EditorLayer.h"

namespace kailux
{
    class Editor
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Editor)

        static Editor create();

        template<typename Layer>
        auto& getLayer()
        {
            return std::get<Layer>(*m_ActiveLayer);
        }

        template<typename Layer>
        const auto& getLayer() const
        {
            return std::get<Layer>(*m_ActiveLayer);
        }

        void render(Scene& scene);

        void update();

    private:
        void createLayers();

        using LayerTypes = std::variant<EditorLayer>;
        Scoped<LayerTypes> m_ActiveLayer;
    };
}
