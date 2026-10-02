#include "Layer.h"

namespace kailux
{
    Layer::Layer(std::string_view name) : mName(name)
    {
    }

    void Layer::OnAttach()
    {
    }

    void Layer::OnDetach()
    {
    }

    void Layer::OnUpdate(float deltaTime)
    {
    }

    void Layer::OnImGuiRender(Scene &scene)
    {
    }

    bool Layer::OnEvent(const Event &event)
    {
        return false;
    }

    std::string_view Layer::GetName() const
    {
        return mName;
    }
}
