#pragma once
#include "Context.h"
#include "SwapChain.h"
#include "FrameData.h"
#include "imgui_backend/ImGuiBackend.h"

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

        void run(Window& window);

    private:
        void submit(const FrameData& frame, vk::Semaphore imageAvailableSemaphore, vk::Semaphore renderFinishedSemaphore) const;
        void render(Window &window);
        void recordImGuiData(const FrameData& frame);

        static constexpr uint32_t s_FramesInFlight = 2;

        Context                                 m_Context;
        SwapChain                               m_SwapChain;
        ImGuiBackend                            m_ImGuiBackend;
        std::array<FrameData, s_FramesInFlight> m_Frames;
        uint32_t                                m_CurrentFrame;
    };
}
