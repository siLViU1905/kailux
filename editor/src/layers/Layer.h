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

        std::vector<std::unique_ptr<Panel>>&       getPanels();
        const std::vector<std::unique_ptr<Panel>>& getPanels() const;

    protected:
        std::vector<std::unique_ptr<Panel>> m_Panels;
    };
}
