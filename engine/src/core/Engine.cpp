#include "Engine.h"

#include "CommandRecorder.h"

namespace kailux
{
    Engine::Engine() : m_CurrentFrame(0)
    {
    }

    Engine::Engine(Engine &&other) noexcept : m_Context(std::move(other.m_Context)),
                                              m_SwapChain(std::move(other.m_SwapChain)),
                                              m_ImGuiBackend(std::move(other.m_ImGuiBackend)),
                                              m_Frames(std::move(other.m_Frames)),
                                              m_CurrentFrame(other.m_CurrentFrame)
    {
    }

    Engine &Engine::operator=(Engine &&other) noexcept
    {
        if (this != &other)
        {
            m_Context = std::move(other.m_Context);
            m_SwapChain = std::move(other.m_SwapChain);
            m_ImGuiBackend = std::move(other.m_ImGuiBackend);
            m_Frames = std::move(other.m_Frames);
            m_CurrentFrame = other.m_CurrentFrame;
        }
        return *this;
    }

    Engine Engine::create(Window &window)
    {
        Engine engine;
        engine.m_Context = Context::create(window);
        engine.m_SwapChain = SwapChain::create(window, engine.m_Context);
        engine.m_ImGuiBackend = ImGuiBackend::create(window, engine.m_Context, engine.m_SwapChain);

        for (auto &frame: engine.m_Frames)
            frame = FrameData::create(engine.m_Context);

        return engine;
    }

    void Engine::submit(const FrameData &frame, vk::Semaphore imageAvailableSemaphore,
                        vk::Semaphore renderFinishedSemaphore) const
    {
        vk::SemaphoreSubmitInfo waitInfo{
            imageAvailableSemaphore,
            1,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput
        };

        vk::SemaphoreSubmitInfo signalInfo{
            renderFinishedSemaphore,
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

        auto acquired = m_SwapChain.acquire();
        if (!acquired)
        {
            m_SwapChain.recreate(window, m_Context);
            return;
        }
        vk::Semaphore renderFinishedSemaphore = m_SwapChain.getPresentSemaphore(acquired->imageIndex); {
            CommandRecorder recorder(frame.getCommandBuffer());

            recorder.barrier(
                {
                    m_SwapChain.getImage(acquired->imageIndex),
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eColorAttachmentOptimal
                }
            );

            recorder.beginRendering(
                {
                    m_SwapChain.getImageView(acquired->imageIndex),
                    m_SwapChain.getExtent(),
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    {std::array{0.1f, 0.1f, 0.1f, 1.0f}},
                    m_SwapChain.getDepthImageView(),
                    vk::ImageLayout::eDepthAttachmentOptimal,
                    vk::RenderingFlagBits::eContentsSecondaryCommandBuffers
                }
            );

            recorder.setViewport(m_SwapChain.getExtent());
            recorder.setScissor(m_SwapChain.getExtent());

            recordImGuiData(frame);

            recorder.getCommandBuffer().executeCommands(frame.getImGuiCommandBuffer());

            recorder.endRendering();

            recorder.barrier(
                {
                    m_SwapChain.getImage(acquired->imageIndex),
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::ImageLayout::ePresentSrcKHR,
                    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                    vk::PipelineStageFlagBits2::eBottomOfPipe,
                    vk::AccessFlagBits2::eColorAttachmentWrite,
                    vk::AccessFlagBits2::eNone
                }
            );
        }

        submit(m_Frames[m_CurrentFrame], acquired->imageAvailableSemaphore, renderFinishedSemaphore);

        if (!m_SwapChain.present(m_Context, acquired->imageIndex, renderFinishedSemaphore))
            m_SwapChain.recreate(window, m_Context);

        m_CurrentFrame = (m_CurrentFrame + 1) % s_FramesInFlight;
    }

    void Engine::recordImGuiData(const FrameData& frame)
    {
        auto format = m_SwapChain.getFormat();
        auto inheritanceInfo = vk::CommandBufferInheritanceRenderingInfo(
            {},
            {},
            1,
            &format,
            m_SwapChain.getDepthFormat(),
            vk::Format::eUndefined,
            vk::SampleCountFlagBits::e1
        );

        m_ImGuiBackend.beginFrame();
        //imgui render commands go here
        m_ImGuiBackend.endFrame();

        CommandRecorder recorder(frame.getImGuiCommandBuffer(), inheritanceInfo);
        m_ImGuiBackend.recordDrawData(recorder.getCommandBuffer());
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
