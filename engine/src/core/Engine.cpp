#include "Engine.h"

#include "FileDialog.h"
#include "command/CommandRecorder.h"
#include "Logger.h"
#include "command/OneTimeCommand.h"
#include "components/entt/CameraComponent.h"
#include "components/entt/MeshComponent.h"
#include "components/gpu/CameraData.h"
#include "components/gpu/MeshData.h"
#include "components/gpu/MeshTransformData.h"
#include "texture/TextureAllocator.h"

namespace kailux
{
    Engine::Engine() : m_SampleCount(vk::SampleCountFlagBits::e1), m_CurrentFrame(0)
    {
    }

    Engine::Engine(Engine &&other) noexcept : m_Context(std::move(other.m_Context)),
                                              m_SampleCount(other.m_SampleCount),
                                              m_Swapchain(std::move(other.m_Swapchain)),
                                              m_ImGuiBackend(std::move(other.m_ImGuiBackend)),
                                              m_DescriptorLayout(std::move(other.m_DescriptorLayout)),
                                              m_DescriptorPool(std::move(other.m_DescriptorPool)),
                                              m_Pipeline(std::move(other.m_Pipeline)),
                                              m_MeshRegistry(std::move(other.m_MeshRegistry)),
                                              m_Frames(std::move(other.m_Frames)),
                                              m_CurrentFrame(other.m_CurrentFrame),
                                              m_Clock(other.m_Clock),
                                              m_Scene(std::move(other.m_Scene)),
                                              m_Skybox(std::move(other.m_Skybox)),
                                              m_PendingData(std::move(other.m_PendingData))
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
            m_DescriptorLayout = std::move(other.m_DescriptorLayout);
            m_DescriptorPool = std::move(other.m_DescriptorPool);
            m_Pipeline = std::move(other.m_Pipeline);
            m_MeshRegistry = std::move(other.m_MeshRegistry);
            m_Frames = std::move(other.m_Frames);
            m_CurrentFrame = other.m_CurrentFrame;
            m_Clock = other.m_Clock;
            m_Scene = std::move(other.m_Scene);
            m_Skybox = std::move(other.m_Skybox);
            m_PendingData = std::move(other.m_PendingData);
        }
        return *this;
    }

    Engine::~Engine()
    {
        if (m_Context.getDevice())
        {
            waitIdle();
            OneTimeCommand::destroy_command_pool();
        }
    }

    Engine Engine::create(Window &window)
    {
        Engine engine;
        engine.createRenderingContext(window);
        OneTimeCommand::create_command_pool(engine.m_Context);
        engine.createDescriptorResources();
        engine.createPipeline();
        engine.createSkybox();
        engine.createFrameResources();
        engine.createMeshRegistry();
        engine.createImGui(window);
        engine.createSceneEntities(window);
        return engine;
    }

    void Engine::setOnEditorRender(OnEditorRender &&callback)
    {
        m_OnEditorRender = std::move(callback);
    }

    void Engine::waitIdle() const
    {
        m_Context.getDevice().waitIdle();
    }

    Queue<MeshRegistry::MeshData> &Engine::getPendingDataQueue()
    {
        return m_PendingData;
    }

    void Engine::createRenderingContext(Window &window)
    {
        m_Context = Context::create(window);
        m_SampleCount = m_Context.getMaxUsableSampleCount();
        m_Swapchain = Swapchain::create(window, m_Context, m_SampleCount);
    }

    void Engine::createDescriptorResources()
    {
        static_assert(
            check_descriptor_layout_bindings_and_pool_sizes_match(s_DescriptorLayoutBindings, s_DescriptorPoolSizes),
            "Descriptor layout binding and pool sizes does not match");
        m_DescriptorLayout = DescriptorLayout::create(m_Context, s_DescriptorLayoutBindings);

        m_DescriptorPool = DescriptorPool::create(m_Context, s_FramesInFlight, s_DescriptorPoolSizes);
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

    void Engine::createSkybox()
    {
        m_Skybox = SkyboxPass::create(
            m_Context,
            m_Swapchain,
            s_FramesInFlight,
            s_SkyboxTexturePaths
        );
    }

    void Engine::createFrameResources()
    {
        for (auto &frame: m_Frames)
            frame = FrameData::create(m_Context, m_DescriptorLayout, m_DescriptorPool, m_Skybox, s_MaxMeshCount);
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

    void Engine::createScene()
    {
        m_Scene = Scene::create();
    }

    void Engine::createSceneEntities(const Window &window)
    {
        int windowWidth, windowHeight;
        window.getFramebufferSize(windowWidth, windowHeight);
        auto cameraEntity = m_Scene.createCameraEntity(
            "MainCamera",
            Camera::create(
                windowWidth,
                windowHeight,
                {0.f, 0.f, 5.f}),
            true
        );
        m_Scene.setMainCamera(cameraEntity);

        m_Scene.createMeshEntity("Cube", m_MeshRegistry.getBuiltins().cube, {}, {});
        m_Scene.createMeshEntity("Sphere", m_MeshRegistry.getBuiltins().sphere, MeshTransformData({1.5f, 0.f, 0.f}),
                                 {});
    }

    PipelineInfo Engine::make_pipeline_info(vk::SampleCountFlagBits sampleCount)
    {
        PipelineInfo info;

        info.vertexInputBinding = Vertex::get_binding_description();
        constexpr auto vertexAttribDesc = Vertex::get_attribute_description();
        info.vertexInputAttribute = {vertexAttribDesc.cbegin(), vertexAttribDesc.cend()};

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

            const auto &ambient = m_Scene.getAmbient();
            vk::ClearColorValue clearColor(ambient.x, ambient.y, ambient.z, ambient.w);
            recorder.beginRendering(
                {
                    m_Swapchain.getColorImageView(),
                    m_Swapchain.getImageView(acquired->imageIndex),
                    m_Swapchain.getExtent(),
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    clearColor,
                    m_Swapchain.getDepthImageView(),
                    vk::ImageLayout::eDepthAttachmentOptimal,
                    {}
                }
            );

            recorder.setViewport(m_Swapchain.getExtent());
            recorder.setScissor(m_Swapchain.getExtent());

            recordMeshData(frame, recorder);

            m_Skybox.render(
                recorder.getCommandBuffer(),
                frame.getSkyboxDescriptorSet(),
                m_MeshRegistry.view(m_MeshRegistry.getBuiltins().cube)
            );

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

        auto meshCount = static_cast<uint32_t>(
            m_Scene.getEntityRegistry().view<MeshComponent>().size()
        );

        recorder.drawIndexedIndirect(
            frame.getIndirectBuffer(),
            meshCount
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
        m_OnEditorRender(m_Scene);
        m_ImGuiBackend.endFrame();

        CommandRecorder recorder(frame.getImGuiCommandBuffer(), inheritanceInfo);
        m_ImGuiBackend.recordDrawData(recorder.getCommandBuffer());
    }

    void Engine::updateFrameBuffers(FrameData &frame, const CommandRecorder &recorder) const
    {
        updateCameraBuffer(frame);
        updateMeshDataBuffer(frame);
        updateIndirectBuffer(frame);
        updateSceneBuffer(frame);
        recorder.bufferMemoryBarriers(frame.getBufferMemoryBarriers());
    }

    void Engine::updateCameraBuffer(FrameData &frame) const
    {
        const auto &camera = m_Scene.getEntityRegistry().get<CameraComponent>(m_Scene.getMainCamera()).camera;
        const auto &lastData = m_Scene.getEntityRegistry().get<CameraData>(m_Scene.getMainCamera());
        CameraData data(
            camera.getProjection(),
            camera.getView(),
            glm::vec4(camera.getPosition(), lastData.positionAndExposure.w)
        );
        frame.getCameraBuffer().upload(
            &data,
            sizeof(CameraData)
        );
    }

    void Engine::updateMeshDataBuffer(FrameData &frame) const
    {
        std::vector<MeshData> data;
        auto view = m_Scene.getEntityRegistry().view<MeshTransformData, MeshMaterialData, MeshComponent>();
        data.reserve(view.size_hint());
        for (auto entity: view)
        {
            const auto &transform = view.get<MeshTransformData>(entity);
            const auto &material = view.get<MeshMaterialData>(entity);
            data.emplace_back(
                transform.getModelMatrix(),
                material
            );
        }
        frame.getModelBuffer().upload(data.data(), data.size() * sizeof(MeshData));
    }

    void Engine::updateIndirectBuffer(FrameData &frame) const
    {
        std::vector<vk::DrawIndexedIndirectCommand> indirectCommands;
        auto view = m_Scene.getEntityRegistry().view<MeshComponent>();
        indirectCommands.reserve(view.size());
        view.each([this, &indirectCommands](auto mesh)
        {
            auto meshView = m_MeshRegistry.view(mesh.handle);
            indirectCommands.emplace_back(
                meshView.indexCount,
                1,
                meshView.firstIndex,
                meshView.vertexOffset,
                0
            );
        });
        frame.getIndirectBuffer().upload(indirectCommands.data(),
                                         indirectCommands.size() * sizeof(vk::DrawIndexedIndirectCommand));
    }

    void Engine::updateSceneBuffer(FrameData &frame) const
    {
        auto data = m_Scene.getData();
        frame.getSceneBuffer().upload(&data, sizeof(SceneData));
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
                            auto view = m_Scene.getEntityRegistry().view<CameraComponent>();
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

    void Engine::pollPendingData()
    {
        if (auto data = m_PendingData.tryPop())
        {
            std::vector<Buffer> stagingBuffers;
            auto otc = OneTimeCommand::create(m_Context);
            auto handle = m_MeshRegistry.upload(m_Context, otc.getCommandBuffer(), *data, stagingBuffers);
            m_Scene.createMeshEntity(m_Scene.getMeshEntityName(), handle, MeshTransformData({-2.f, 0.f, 0.f}), {});

            otc.submit(m_Context);
        }
    }

    void Engine::run(Window &window)
    {
        m_Clock.tick();
        handleEvent(window);
        pollPendingData();

        auto deltaTime = m_Clock.getDeltaTime<float, TimeType::Seconds>();
        auto view = m_Scene.getEntityRegistry().view<CameraComponent>();
        for (auto entity: view)
        {
            auto &camera = view.get<CameraComponent>(entity).camera;
            camera.updateMovement(window, deltaTime);
            camera.updateLookAt(window, deltaTime);
        }

        if (window.isMinimized())
            return;

        if (window.wasResized())
            m_Swapchain.recreate(window, m_Context, m_SampleCount);

        render(window);
    }
}
