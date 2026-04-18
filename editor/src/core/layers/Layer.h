#pragma once
#include "../panels/Panel.h"

namespace kailux
{
    class Layer
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Layer)
        virtual ~Layer() = default;

        void render(Scene& scene) const;

        std::vector<Scoped<Panel>>       &getPanels();
        const std::vector<Scoped<Panel>> &getPanels() const;

    protected:
        std::vector<Scoped<Panel>> m_Panels;
    };
}
