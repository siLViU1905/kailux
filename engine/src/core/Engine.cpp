#include "Engine.h"

#include "command/CommandRecorder.h"
#include "Logger.h"
#include "command/OneTimeCommand.h"

namespace kailux
{
    Engine::Engine() : m_SampleCount(vk::SampleCountFlagBits::e1), m_CurrentFrame(0)
    {
    }

    Engine::Engine(Engine &&other) noexcept : m_Context(std::move(other.m_Context)),
                                              m_SampleCount(other.m_SampleCount),
                                              m_Swapchain(std::move(other.m_Swapchain)),
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
            m_SampleCount = other.m_SampleCount;
            m_Swapchain = std::move(other.m_Swapchain);
            m_ImGuiBackend = std::move(other.m_ImGuiBackend);
            m_Frames = std::move(other.m_Frames);
            m_CurrentFrame = other.m_CurrentFrame;
        }
        return *this;
    }

    Engine::~Engine()
    {
        OneTimeCommand::destroy_command_pool();
    }

    Engine Engine::create(Window &window)
    {
        Engine engine;
        engine.m_Context = Context::create(window);
        engine.m_SampleCount = engine.m_Context.getMaxUsableSampleCount();
        engine.m_Swapchain = Swapchain::create(window, engine.m_Context, engine.m_SampleCount);
        engine.m_ImGuiBackend =
                ImGuiBackend::create(window, engine.m_Context, engine.m_Swapchain, engine.m_SampleCount);
        engine.m_DescriptorSetLayout = DescriptorLayout::create(engine.m_Context, {});
        engine.m_Pipeline = Pipeline::create(
            engine.m_Context,
            engine.m_Swapchain,
            engine.m_DescriptorSetLayout,
            {s_VertexShaderPath.data(), s_FragmentShaderPath.data()},
            make_pipeline_info(engine.m_SampleCount)
        );

        for (auto &frame: engine.m_Frames)
            frame = FrameData::create(engine.m_Context);

        OneTimeCommand::create_command_pool(engine.m_Context);

        std::vector<Buffer> stagingBuffers;
        OneTimeCommand otc = OneTimeCommand::create(engine.m_Context);
        engine.m_MeshRegistry = MeshRegistry::create(engine.m_Context, otc.getCommandBuffer(), stagingBuffers);
        otc.submit(engine.m_Context);

        return engine;
    }

    PipelineInfo Engine::make_pipeline_info(vk::SampleCountFlagBits sampleCount)
    {
        PipelineInfo info;
        info.topology = vk::PrimitiveTopology::eTriangleList;

        info.rasterizer = {
            {},
            vk::False,
            vk::False,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eNone,//TO DO: no rendering with eBack, perspective matrix needs to be added
            vk::FrontFace::eCounterClockwise,
            vk::False,
            {},
            {},
            1.f,
            1.f
        };

        info.colorBlendAttachment.colorWriteMask =
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eA;
        info.colorBlendAttachment.blendEnable = vk::True;
        info.colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        info.colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        info.colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
        info.colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        info.colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        info.colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

        info.samples = sampleCount;

        info.depthStencilInfo.depthTestEnable = vk::True;
        info.depthStencilInfo.depthWriteEnable = vk::True;
        info.depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
        info.depthStencilInfo.depthBoundsTestEnable = vk::False;
        info.depthStencilInfo.stencilTestEnable = vk::False;

        return info;
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

        auto acquired = m_Swapchain.acquire();
        if (!acquired)
        {
            m_Swapchain.recreate(window, m_Context, m_SampleCount);
            return;
        }
        vk::Semaphore renderFinishedSemaphore = m_Swapchain.getPresentSemaphore(acquired->imageIndex); {
            CommandRecorder recorder(frame.getCommandBuffer());

            recorder.barrier(
                {
                    m_Swapchain.getColorImage(),
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eColorAttachmentOptimal
                }
            );

            recorder.barrier(
                {
                    m_Swapchain.getImage(acquired->imageIndex),
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eColorAttachmentOptimal
                });

            recorder.beginRendering(
                {
                    m_Swapchain.getColorImageView(),
                    m_Swapchain.getImageView(acquired->imageIndex),
                    m_Swapchain.getExtent(),
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    {std::array{0.1f, 0.1f, 0.1f, 1.0f}},
                    m_Swapchain.getDepthImageView(),
                    vk::ImageLayout::eDepthAttachmentOptimal,
                    {}
                }
            );

            recorder.setViewport(m_Swapchain.getExtent());
            recorder.setScissor(m_Swapchain.getExtent());

            recordMeshData(recorder.getCommandBuffer());

            recorder.endRendering();

            recorder.beginRendering(
                {
                    m_Swapchain.getColorImageView(),
                    m_Swapchain.getImageView(acquired->imageIndex),
                    m_Swapchain.getExtent(),
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eLoad,
                    vk::AttachmentStoreOp::eStore,
                    {std::array{0.1f, 0.1f, 0.1f, 1.0f}},
                    m_Swapchain.getDepthImageView(),
                    vk::ImageLayout::eDepthAttachmentOptimal,
                    vk::RenderingFlagBits::eContentsSecondaryCommandBuffers
                }
            );

            recordImGuiData(frame);
            recorder.getCommandBuffer().executeCommands(frame.getImGuiCommandBuffer());

            recorder.endRendering();

            recorder.barrier(
                {
                    m_Swapchain.getImage(acquired->imageIndex),
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

        if (!m_Swapchain.present(m_Context, acquired->imageIndex, renderFinishedSemaphore))
            m_Swapchain.recreate(window, m_Context, m_SampleCount);

        m_CurrentFrame = (m_CurrentFrame + 1) % s_FramesInFlight;
    }

    void Engine::recordMeshData(vk::CommandBuffer cmd) const
    {
        m_Pipeline.bind(cmd);
        m_MeshRegistry.bind(cmd);

        auto mv = m_MeshRegistry.view(m_MeshRegistry.getBuiltins().cube);

        cmd.drawIndexed(
            mv.indexCount,
            1,
            mv.firstIndex,
            mv.vertexOffset,
            0
        );
    }

    void Engine::recordImGuiData(const FrameData &frame)
    {
        auto format = m_Swapchain.getFormat();
        auto inheritanceInfo = vk::CommandBufferInheritanceRenderingInfo(
            {},
            {},
            1,
            &format,
            m_Swapchain.getDepthFormat(),
            vk::Format::eUndefined,
            m_SampleCount
        );

        m_ImGuiBackend.beginFrame();
        ImGui::Begin("Test");
        ImGui::End();
        m_ImGuiBackend.endFrame();

        CommandRecorder recorder(frame.getImGuiCommandBuffer(), inheritanceInfo);
        m_ImGuiBackend.recordDrawData(recorder.getCommandBuffer());
    }

    void Engine::handleEvent(Event event)
    {
        std::visit(
            [](auto e)
            {
                KAILUX_LOG_INFO("[Engine]", e.toString())
            },
            event
        );
    }

    void Engine::run(Window &window)
    {
        while (window.isOpen())
        {
            window.pollEvents();

            if (auto event = window.getEvent())
                handleEvent(*event);

            if (window.isMinimized())
                continue;

            if (window.wasResized())
                m_Swapchain.recreate(window, m_Context, m_SampleCount);

            render(window);
        }

        m_Context.getDevice().waitIdle();
    }
}
