#include "Engine.h"

namespace kailux
{
    Engine::Engine()
    {
    }

    Engine::Engine(Engine &&other) noexcept : m_Context(std::move(other.m_Context)),
                                              m_SwapChain(std::move(other.m_SwapChain))
    {
    }

    Engine &Engine::operator=(Engine &&other) noexcept
    {
        if (this != &other)
        {
            m_Context = std::move(other.m_Context);
            m_SwapChain = std::move(other.m_SwapChain);
        }
        return *this;
    }

    Engine Engine::create(Window &window)
    {
        Engine engine;
        engine.m_Context = Context::create(window);
        engine.m_SwapChain = SwapChain::create(window, engine.m_Context);

        return engine;
    }
}
