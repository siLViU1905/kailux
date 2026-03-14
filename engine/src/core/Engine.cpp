#include "Engine.h"

#include "CommandRecorder.h"

namespace kailux
{
    Engine::Engine() : m_CurrentFrame(0), m_ImageIndex(0)
    {
    }

    Engine::Engine(Engine &&other) noexcept : m_Context(std::move(other.m_Context)),
                                              m_SwapChain(std::move(other.m_SwapChain)),
                                              m_Frames(std::move(other.m_Frames)),
                                              m_CurrentFrame(other.m_CurrentFrame),
                                              m_ImageIndex(other.m_ImageIndex)
    {
    }

    Engine &Engine::operator=(Engine &&other) noexcept
    {
        if (this != &other)
        {
            m_Context = std::move(other.m_Context);
            m_SwapChain = std::move(other.m_SwapChain);
            m_Frames = std::move(other.m_Frames);
            m_CurrentFrame = other.m_CurrentFrame;
            m_ImageIndex = other.m_ImageIndex;
        }
        return *this;
    }

    Engine Engine::create(Window &window)
    {
        Engine engine;
        engine.m_Context = Context::create(window);
        engine.m_SwapChain = SwapChain::create(window, engine.m_Context);

        for (auto &frame: engine.m_Frames)
            frame = FrameData::create(engine.m_Context);

        return engine;
    }

    void Engine::submit(const FrameData &frame) const
    {
        vk::SemaphoreSubmitInfo waitInfo{
            frame.getImageAvailableSemaphore(),
            1,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput
        };

        vk::SemaphoreSubmitInfo signalInfo{
            frame.getRenderFinishedSemaphore(),
            1,
            vk::PipelineStageFlagBits2::eAllGraphics
        };

        vk::CommandBufferSubmitInfo cmdInfo{frame.getCommandBuffer()};

        vk::SubmitInfo2 submitInfo{
            {},
            waitInfo,
            cmdInfo,
            signalInfo
        };

        m_Context.getGraphicsQueue().submit2(submitInfo, frame.getFenceInFlight());
    }

    void Engine::render(Window &window)
    {
        auto &frame = m_Frames[m_CurrentFrame];
        frame.reset(m_Context);

        auto imageIndex = m_SwapChain.acquire(frame.getImageAvailableSemaphore());
        if (!imageIndex)
        {
            m_SwapChain.recreate(window, m_Context);
            return;
        }
        m_ImageIndex = *imageIndex; {
            CommandRecorder recorder(frame.getCommandBuffer());

            recorder.barrier(
                {
                    m_SwapChain.getImage(*imageIndex),
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eColorAttachmentOptimal
                }
            );

            recorder.beginRendering(
                {
                    m_SwapChain.getImageView(*imageIndex),
                    m_SwapChain.getExtent()
                }
            );

            recorder.setViewport(m_SwapChain.getExtent());
            recorder.setScissor(m_SwapChain.getExtent());

            recorder.endRendering();

            recorder.barrier(
                {
                    m_SwapChain.getImage(*imageIndex),
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::ImageLayout::ePresentSrcKHR,
                    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                    vk::PipelineStageFlagBits2::eBottomOfPipe,
                    vk::AccessFlagBits2::eColorAttachmentWrite,
                    vk::AccessFlagBits2::eNone
                }
            );
        }

        submit(m_Frames[m_CurrentFrame]);

        if (!m_SwapChain.present(m_Context, *imageIndex, frame.getRenderFinishedSemaphore()))
            m_SwapChain.recreate(window, m_Context);

        m_CurrentFrame = (m_CurrentFrame + 1) % s_FramesInFlight;
    }

    void Engine::run(Window &window)
    {
        while (window.isOpen())
        {
            window.pollEvents();

            if (window.isMinimized())
                continue;

            if (window.wasResized())
                m_SwapChain.recreate(window, m_Context);

            render(window);
        }

        m_Context.getDevice().waitIdle();
    }
}
