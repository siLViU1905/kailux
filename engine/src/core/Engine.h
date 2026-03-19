#pragma once
#include "Context.h"
#include "Swapchain.h"
#include "FrameData.h"
#include "imgui_backend/ImGuiBackend.h"
#include "window/Event.h"

namespace kailux
{
    class Engine
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Engine)

        static Engine create(Window& window);

        void run(Window& window);

    private:
        void submit(const FrameData& frame, vk::Semaphore imageAvailableSemaphore, vk::Semaphore renderFinishedSemaphore) const;
        void render(Window &window);
        void recordImGuiData(const FrameData& frame);

        void handleEvent(Event event);

        static constexpr uint32_t s_FramesInFlight = 2;

        Context                                 m_Context;
        Swapchain                               m_Swapchain;
        ImGuiBackend                            m_ImGuiBackend;
        std::array<FrameData, s_FramesInFlight> m_Frames;
        uint32_t                                m_CurrentFrame;
    };
}
