#include "Layer.h"

namespace kailux
{
    Layer::Layer() = default;

    Layer::Layer(Layer &&other) noexcept : mPanels(std::move(other.mPanels))
    {
    }

    Layer &Layer::operator=(Layer &&other) noexcept
    {
        if (this != &other)
        {
            mPanels = std::move(other.mPanels);
        }
        return *this;
    }

    void Layer::renderPanels(Scene &scene) const
    {
        for (auto& panel : mPanels)
            if (panel->isOpen())
                panel->render(scene);
    }
}
