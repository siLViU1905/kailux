#include "Engine.h"

#include "command/CommandRecorder.h"
#include "Logger.h"
#include "command/OneTimeCommand.h"
#include "components/CameraComponent.h"

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
                                              m_CurrentFrame(other.m_CurrentFrame),
                                              m_Clock(other.m_Clock),
                                              m_Camera(other.m_Camera)
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
            m_Clock = other.m_Clock;
            m_Camera = other.m_Camera;
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

        auto descLayoutBindings = make_descriptor_layout_bindings(1); //camera uniform buffer
        engine.m_DescriptorLayout = DescriptorLayout::create(engine.m_Context, descLayoutBindings);
        auto descPoolSizes = make_descriptor_pool_sizes(1); //camera uniform buffer
        engine.m_DescriptorPool = DescriptorPool::create(engine.m_Context, s_FramesInFlight, descPoolSizes);
        engine.m_Pipeline = Pipeline::create(
            engine.m_Context,
            engine.m_Swapchain,
            engine.m_DescriptorLayout,
            {s_VertexShaderPath.data(), s_FragmentShaderPath.data()},
            make_pipeline_info(engine.m_SampleCount)
        );

        for (auto &frame: engine.m_Frames)
            frame = FrameData::create(engine.m_Context, engine.m_DescriptorLayout, engine.m_DescriptorPool);

        OneTimeCommand::create_command_pool(engine.m_Context);

        std::vector<Buffer> stagingBuffers;
        OneTimeCommand otc = OneTimeCommand::create(engine.m_Context);
        engine.m_MeshRegistry = MeshRegistry::create(engine.m_Context, otc.getCommandBuffer(), stagingBuffers);
        otc.submit(engine.m_Context);

        int windowWidth, windowHeight;
        window.getFramebufferSize(windowWidth, windowHeight);
        engine.m_Camera = Camera::create(windowWidth, windowHeight, {0.f, 0.f, -2.f});

        return engine;
    }

    std::array<DescriptorLayoutBinding, 1> Engine::make_descriptor_layout_bindings(uint32_t uniformBufferCount)
    {
        return {
            DescriptorLayoutBinding(
                vk::DescriptorType::eUniformBuffer,
                uniformBufferCount,
                vk::ShaderStageFlagBits::eVertex
            )
        };
    }

    std::array<DescriptorPoolSize, 1> Engine::make_descriptor_pool_sizes(uint32_t uniformBufferCount)
    {
        return {
            DescriptorPoolSize(
                vk::DescriptorType::eUniformBuffer,
                uniformBufferCount
            )
        };
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
            vk::CullModeFlagBits::eNone, //TO DO: no rendering with eBack, perspective matrix needs to be added
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
        updateFrameBuffers(frame);

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

            recordMeshData(frame, recorder.getCommandBuffer());

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

    void Engine::recordMeshData(const FrameData &frame, vk::CommandBuffer cmd) const
    {
        m_Pipeline.bind(cmd);
        m_MeshRegistry.bind(cmd);
        frame.getDescriptorSet().bind(m_Pipeline, cmd);

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

    void Engine::updateFrameBuffers(FrameData &frame) const
    {
        CameraComponent cameraComponent(
            m_Camera.getProjection(),
            m_Camera.getView(),
            {m_Camera.getPosition(), 0.f}
        );
        frame.getCameraBuffer().upload(&cameraComponent, sizeof(CameraComponent));
    }

    void Engine::handleEvent(Event event)
    {
        std::visit(EventOverloads{
                       [](KeyPressed e)
                       {
                           KAILUX_LOG_INFO("[Engine]", e.toString())
                       },
                       [](KeyReleased e)
                       {
                           KAILUX_LOG_INFO("[Engine]", e.toString())
                       },
                       [](KeyRepeated e)
                       {
                           KAILUX_LOG_INFO("[Engine]", e.toString())
                       },
                       [](ButtonPressed e)
                       {
                           KAILUX_LOG_INFO("[Engine]", e.toString())
                       },
                       [](ButtonReleased e)
                       {
                           KAILUX_LOG_INFO("[Engine]", e.toString())
                       }
                   },
                   event
        );
    }

    void Engine::run(Window &window)
    {
        while (window.isOpen())
        {
            m_Clock.tick();
            window.pollEvents();

            if (auto event = window.getEvent())
                handleEvent(*event);

            auto deltaTime = m_Clock.getDeltaTime<float, TimeType::Seconds>();
            m_Camera.updateMovement(window, deltaTime);
            m_Camera.updateLookAt(window, deltaTime);

            if (window.isMinimized())
                continue;

            if (window.wasResized())
                m_Swapchain.recreate(window, m_Context, m_SampleCount);

            render(window);
        }

        m_Context.getDevice().waitIdle();
    }
}
