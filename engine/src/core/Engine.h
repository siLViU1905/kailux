#pragma once
#include "Context.h"
#include "SwapChain.h"

namespace kailux
{
    class Engine
    {
    public:
        Engine();
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;
        Engine(Engine&& other) noexcept;
        Engine& operator=(Engine&& other) noexcept;

        static Engine create(Window& window);

    private:
        Context   m_Context;
        SwapChain m_SwapChain;
    };
}
