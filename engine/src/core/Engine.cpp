#include "Engine.h"

#include "command/CommandRecorder.h"
#include "Logger.h"
#include "command/OneTimeCommand.h"
#include "components/entt/CameraComponent.h"
#include "components/entt/MeshComponent.h"
#include "components/gpu/CameraData.h"
#include "components/gpu/MeshTransformData.h"

namespace kailux
{
    Engine::Engine() : m_SampleCount(vk::SampleCountFlagBits::e1), m_CurrentFrame(0), m_MainCameraEntity()
    {
    }

    Engine::Engine(Engine &&other) noexcept : m_Context(std::move(other.m_Context)),
                                              m_SampleCount(other.m_SampleCount),
                                              m_Swapchain(std::move(other.m_Swapchain)),
                                              m_ImGuiBackend(std::move(other.m_ImGuiBackend)),
                                              m_Frames(std::move(other.m_Frames)),
                                              m_CurrentFrame(other.m_CurrentFrame),
                                              m_Clock(other.m_Clock),
                                              m_EntityRegistry(std::move(other.m_EntityRegistry)),
                                              m_MainCameraEntity(other.m_MainCameraEntity)
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
            m_EntityRegistry = std::move(other.m_EntityRegistry);
            m_MainCameraEntity = other.m_MainCameraEntity;
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
        engine.createRenderingContext(window);
        OneTimeCommand::create_command_pool(engine.m_Context);
        engine.createDescriptorResources();
        engine.createPipeline();
        engine.createFrameResources();
        engine.createMeshRegistry();
        engine.createImGui(window);
        engine.createEntities(window);
        return engine;
    }

    void Engine::createRenderingContext(Window &window)
    {
        m_Context = Context::create(window);
        m_SampleCount = m_Context.getMaxUsableSampleCount();
        m_Swapchain = Swapchain::create(window, m_Context, m_SampleCount);
    }

    void Engine::createDescriptorResources()
    {
        constexpr auto descLayoutBindings = make_descriptor_layout_bindings(1); //camera uniform buffer
        constexpr auto descPoolSizes = make_descriptor_pool_sizes(1); //camera uniform buffer
        static_assert(check_descriptor_layout_bindings_and_pool_sizes_match(descLayoutBindings, descPoolSizes),
                      "Descriptor layout binding and pool sizes does not match");
        m_DescriptorLayout = DescriptorLayout::create(m_Context, descLayoutBindings);

        m_DescriptorPool = DescriptorPool::create(m_Context, s_FramesInFlight, descPoolSizes);
    }

    void Engine::createPipeline()
    {
        m_Pipeline = Pipeline::create(
            m_Context,
            m_Swapchain,
            m_DescriptorLayout,
            {
                s_VertexShaderPath.data(),
                s_FragmentShaderPath.data()
            },
            make_pipeline_info(m_SampleCount)
        );
    }

    void Engine::createFrameResources()
    {
        for (auto &frame: m_Frames)
            frame = FrameData::create(m_Context, m_DescriptorLayout, m_DescriptorPool, s_MaxMeshCount);
    }

    void Engine::createMeshRegistry()
    {
        std::vector<Buffer> stagingBuffers;
        OneTimeCommand otc = OneTimeCommand::create(m_Context);
        m_MeshRegistry = MeshRegistry::create(m_Context, otc.getCommandBuffer(), stagingBuffers);
        otc.submit(m_Context);
    }

    void Engine::createImGui(Window &window)
    {
        m_ImGuiBackend = ImGuiBackend::create(window, m_Context, m_Swapchain, m_SampleCount);
    }

    void Engine::createEntities(const Window &window)
    {
        m_MainCameraEntity = m_EntityRegistry.create();

        int windowWidth, windowHeight;
        window.getFramebufferSize(windowWidth, windowHeight);
        m_EntityRegistry.emplace<CameraComponent>(
            m_MainCameraEntity,
            Camera::create(windowWidth, windowHeight, {0.f, 0.f, 5.f}),
            true
        );

        auto createBuiltinEntity = [this](auto handle)
        {
            auto entity = m_EntityRegistry.create();
            m_EntityRegistry.emplace<MeshComponent>(entity, handle);
            m_EntityRegistry.emplace<MeshTransformData>(entity);
        };

        createBuiltinEntity(m_MeshRegistry.getBuiltins().cube);
        createBuiltinEntity(m_MeshRegistry.getBuiltins().sphere);
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
            vk::CullModeFlagBits::eBack,
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
            updateFrameBuffers(frame, recorder);

            recorder.imageBarrier(
                {
                    m_Swapchain.getColorImage(),
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eColorAttachmentOptimal
                }
            );

            recorder.imageBarrier(
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

            recordMeshData(frame, recorder);

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

            recorder.imageBarrier(
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

    std::vector<vk::DrawIndexedIndirectCommand> Engine::getMeshIndirectCommands() const
    {
        auto views = m_MeshRegistry.viewAll();
        std::vector<vk::DrawIndexedIndirectCommand> commands;
        commands.reserve(views.size());
        for (auto view: views)
            commands.emplace_back(
                view.indexCount,
                1,
                view.firstIndex,
                view.vertexOffset,
                0
            );
        return commands;
    }

    void Engine::recordMeshData(const FrameData &frame, const CommandRecorder &recorder) const
    {
        m_Pipeline.bind(recorder.getCommandBuffer());
        m_MeshRegistry.bind(recorder.getCommandBuffer());
        frame.getDescriptorSet().bind(m_Pipeline, recorder.getCommandBuffer());

        recorder.drawIndexedIndirect(
            frame.getIndirectBuffer(),
            m_MeshRegistry.getMeshCount()
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

    void Engine::updateFrameBuffers(FrameData &frame, const CommandRecorder &recorder) const
    {
        const auto &camera = m_EntityRegistry.get<CameraComponent>(m_MainCameraEntity).camera;
        CameraData cameraData(
            camera.getProjection(),
            camera.getView(),
            {camera.getPosition(), 0.f}
        );
        frame.getCameraBuffer().upload(&cameraData, sizeof(CameraData));

        auto indirectCommands = getMeshIndirectCommands();
        frame.getIndirectBuffer().upload(indirectCommands.data(),
                                         indirectCommands.size() * sizeof(vk::DrawIndexedIndirectCommand));

        recorder.bufferMemoryBarriers(frame.getBufferMemoryBarriers());
    }

    void Engine::handleEvent(Window &window)
    {
        if (auto event = window.getEvent())
            std::visit(
                EventOverloads
                {
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
                    [this, &window](ButtonPressed e)
                    {
                        KAILUX_LOG_INFO("[Engine]", e.toString())
                        if (e.button == MouseButton::Middle)
                        {
                            (window.getCursorMode() == CursorMode::Normal)
                                ? window.setCursorMode(CursorMode::Disabled)
                                : window.setCursorMode(CursorMode::Normal);
                            auto view = m_EntityRegistry.view<CameraComponent>();
                            for (auto entity: view)
                            {
                                auto &camera = view.get<CameraComponent>(entity).camera;
                                camera.isFocused() ? camera.loseFocus() : camera.gainFocus();
                            }
                        }
                    },
                    [](ButtonReleased e)
                    {
                        KAILUX_LOG_INFO("[Engine]", e.toString())
                    }
                },
                *event
            );
    }

    void Engine::run(Window &window)
    {
        while (window.isOpen())
        {
            m_Clock.tick();
            window.pollEvents();
            handleEvent(window);

            auto deltaTime = m_Clock.getDeltaTime<float, TimeType::Seconds>();
            auto view = m_EntityRegistry.view<CameraComponent>();
            for (auto entity: view)
            {
                auto &camera = view.get<CameraComponent>(entity).camera;
                camera.updateMovement(window, deltaTime);
                camera.updateLookAt(window, deltaTime);
            }

            if (window.isMinimized())
                continue;

            if (window.wasResized())
                m_Swapchain.recreate(window, m_Context, m_SampleCount);

            render(window);
        }

        m_Context.getDevice().waitIdle();
    }
}
