#include "Layer.h"

namespace kailux
{
    Layer::Layer()
    {
    }

    Layer::Layer(Layer &&other) noexcept : m_Panels(std::move(other.m_Panels))
    {
    }

    Layer & Layer::operator=(Layer &&other) noexcept
    {
        if (this != &other)
        {
            m_Panels = std::move(other.m_Panels);
        }
        return *this;
    }

    void Layer::render(Scene &scene) const
    {
        for (auto &panel: m_Panels)
            panel->render(scene);
    }

    std::vector<std::unique_ptr<Panel> > &Layer::getPanels()
    {
        return m_Panels;
    }

    const std::vector<std::unique_ptr<Panel> > &Layer::getPanels() const
    {
        return m_Panels;
    }
}
