#include "Engine.h"

#include <fstream>
#include <magic_enum/magic_enum.hpp>

#include "FileDialog.h"
#include "Geometry.h"
#include "command/CommandRecorder.h"
#include "Log.h"
#include "command/OneTimeCommand.h"
#include "components/entt/CameraComponent.h"
#include "components/entt/MaterialComponent.h"
#include "components/entt/MeshComponent.h"
#include "components/gpu/CameraData.h"
#include "components/gpu/MeshData.h"
#include "components/gpu/MeshTransformData.h"
#include "texture/TextureAllocator.h"

namespace kailux
{
    Engine::Engine() : m_SampleCount(vk::SampleCountFlagBits::e1), m_CurrentFrame(0), m_PickedEntity(~0u)
    {
    }

    Engine::Engine(Engine &&other) noexcept : m_Context(std::move(other.m_Context)),
                                              m_SampleCount(other.m_SampleCount),
                                              m_Swapchain(std::move(other.m_Swapchain)),
                                              m_ImGuiBackend(std::move(other.m_ImGuiBackend)),
                                              m_MeshRegistry(std::move(other.m_MeshRegistry)),
                                              m_TextureRegistry(std::move(other.m_TextureRegistry)),
                                              m_Frames(std::move(other.m_Frames)),
                                              m_CurrentFrame(other.m_CurrentFrame),
                                              m_SceneTextureIds(other.m_SceneTextureIds),
                                              m_Scene(std::move(other.m_Scene)),
                                              m_MainPass(std::move(other.m_MainPass)),
                                              m_SkyboxPass(std::move(other.m_SkyboxPass)),
                                              m_OutlinePass(std::move(other.m_OutlinePass)),
                                              m_ComputePicker(std::move(other.m_ComputePicker)),
                                              m_PickedEntity(other.m_PickedEntity),
                                              m_ComputeCuller(std::move(other.m_ComputeCuller)),
                                              m_PendingMeshData(std::move(other.m_PendingMeshData)),
                                              m_MeshCache(std::move(other.m_MeshCache)),
                                              m_PendingFrameTasks(std::move(other.m_PendingFrameTasks)),
                                              m_OnInfoLog(std::move(other.m_OnInfoLog)),
                                              m_OnWarningLog(std::move(other.m_OnWarningLog)),
                                              m_OnErrorLog(std::move(other.m_OnErrorLog))
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
            m_MeshRegistry = std::move(other.m_MeshRegistry);
            m_TextureRegistry = std::move(other.m_TextureRegistry);
            m_Frames = std::move(other.m_Frames);
            m_CurrentFrame = other.m_CurrentFrame;
            m_SceneTextureIds = other.m_SceneTextureIds;
            m_Scene = std::move(other.m_Scene);
            m_MainPass = std::move(other.m_MainPass);
            m_SkyboxPass = std::move(other.m_SkyboxPass);
            m_OutlinePass = std::move(other.m_OutlinePass);
            m_ComputePicker = std::move(other.m_ComputePicker);
            m_PickedEntity = other.m_PickedEntity;
            m_ComputeCuller = std::move(other.m_ComputeCuller);
            m_PendingMeshData = std::move(other.m_PendingMeshData);
            m_MeshCache = std::move(other.m_MeshCache);
            m_PendingFrameTasks = std::move(other.m_PendingFrameTasks);
            m_OnInfoLog = std::move(other.m_OnInfoLog);
            m_OnWarningLog = std::move(other.m_OnWarningLog);
            m_OnErrorLog = std::move(other.m_OnErrorLog);
        }
        return *this;
    }

    Engine::~Engine()
    {
        if (m_Context.getDevice())
        {
            waitIdle();
            m_Frames = {};
            OneTimeCommand::destroy_command_pool();
        }
    }

    Engine Engine::create(Window &window)
    {
        Engine engine;
        engine.createRenderingContext(window);
        OneTimeCommand::create_command_pool(engine.m_Context);
        engine.createMainPass();
        engine.createSkybox();
        engine.createOutlinePass();
        engine.createMeshRegistry();
        engine.createTextureRegistry();
        engine.createComputePicker();
        engine.createComputeCuller();
        engine.createFrameResources();
        engine.createImGui(window);
        engine.createSceneTextureIds();
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

    Queue<Engine::PendingMeshData> &Engine::getPendingMeshDataQueue()
    {
        return m_PendingMeshData;
    }

    void Engine::unregisterMesh(MeshHandle handle, std::string_view path)
    {
        if (uncacheMesh(path))
            m_MeshRegistry.destroy(handle);
    }

    void Engine::unregisterTextureSet(TextureSetHandle handle)
    {
        auto defaultHandle = m_TextureRegistry.getDefaultSetHandle();
        const auto &defaultSet = m_TextureRegistry.view(defaultHandle);
        auto updateInfos = make_descriptor_set_update_info_from_texture_set(defaultHandle, defaultSet);

        for (const auto &frame: m_Frames)
            frame.getDescriptorSet().updateInfo(
                m_Context,
                updateInfos
            );

        m_PendingFrameTasks.emplace_back(
            [this, handle]()
            {
                m_TextureRegistry.unregisterTextureSet(handle);
            }
        );
    }

    ImTextureID Engine::getAssetBrowserDirectoryTextureId() const
    {
        return ImGuiBackend::get_texture_id_from_texture(m_TextureRegistry.getAssetBrowserDirectoryIconTexture());
    }

    ImTextureID Engine::getAssetBrowserFileTextureId() const
    {
        return ImGuiBackend::get_texture_id_from_texture(m_TextureRegistry.getAssetBrowserFileIconTexture());
    }

    ImTextureID Engine::getSceneTextureId() const
    {
        return m_SceneTextureIds[m_CurrentFrame];
    }

    void Engine::createRenderingContext(Window &window)
    {
        m_Context = Context::create(window);
        m_SampleCount = m_Context.getMaxUsableSampleCount();
        m_Swapchain = Swapchain::create(window, m_Context, m_SampleCount);
    }

    void Engine::createMainPass()
    {
        m_MainPass = MainPass::create(
            m_Context,
            m_Swapchain,
            s_FramesInFlight
        );
    }

    void Engine::createSkybox()
    {
        m_SkyboxPass = SkyboxPass::create(
            m_Context,
            m_Swapchain,
            s_FramesInFlight
        );
    }

    void Engine::createOutlinePass()
    {
        m_OutlinePass = OutlinePass::create(
            m_Context,
            m_Swapchain,
            s_FramesInFlight
        );
    }

    void Engine::createFrameResources()
    {
        for (auto &frame: m_Frames)
            frame = FrameData::create(
                m_Context,
                m_Swapchain,
                m_MainPass,
                m_SkyboxPass,
                m_ComputePicker,
                m_OutlinePass,
                m_ComputeCuller,
                m_TextureRegistry, s_MaxMeshCount
            );
    }

    void Engine::createMeshRegistry()
    {
        std::vector<Buffer> stagingBuffers;
        OneTimeCommand otc = OneTimeCommand::create(m_Context);
        m_MeshRegistry = MeshRegistry::create(m_Context, otc.getCommandBuffer(), stagingBuffers);
        otc.submit(m_Context);
    }

    void Engine::createTextureRegistry()
    {
        m_TextureRegistry = TextureRegistry::create(
            m_Context,
            s_MaxMeshCount,
            s_DirectoryIconPath,
            s_FileIconPath
        );
    }

    void Engine::createImGui(Window &window)
    {
        m_ImGuiBackend = ImGuiBackend::create(window, m_Context, m_Swapchain, m_SampleCount);
    }

    void Engine::createSceneTextureIds()
    {
        for (uint32_t i = 0; i < s_FramesInFlight; i++)
            m_SceneTextureIds[i] = ImGuiBackend::get_texture_id_from_texture(m_Frames[i].getSceneTexture());
    }

    void Engine::createComputePicker()
    {
        m_ComputePicker = ComputePicker::create(m_Context, s_FramesInFlight);
    }

    void Engine::createComputeCuller()
    {
        m_ComputeCuller = ComputeCuller::create(m_Context, s_FramesInFlight);
    }

    void Engine::createScene()
    {
        m_Scene = Scene::create("MainScene");
    }

    void Engine::createSceneEntities(const Window &window)
    {
        int windowWidth, windowHeight;
        window.getFramebufferSize(windowWidth, windowHeight);
        auto cameraEntity = m_Scene.createCameraEntity(
            "MainCamera",
            true,
            windowWidth,
            windowHeight
        );
        m_Scene.setMainCamera(cameraEntity);

        m_Scene.createMeshEntity(
            "Cube",
            {
                m_MeshRegistry.getBuiltins().cube,
                "",
                MeshType::Cube,
                Geometry::computeBoundingSphere(MeshRegistry::generate_cube().vertices)
            },
            m_TextureRegistry.getDefaultSetHandle(),
            {},
            {}
        );
        m_Scene.createMeshEntity(
            "Sphere",
            {
                m_MeshRegistry.getBuiltins().sphere,
                "",
                MeshType::Cube,
                Geometry::computeBoundingSphere(MeshRegistry::generate_sphere().vertices)
            },
            m_TextureRegistry.getDefaultSetHandle(),
            MeshTransformData({1.5f, 0.f, 0.f}),
            {}
        );
    }

    std::array<DescriptorSetUpdateInfo, TextureRegistry::s_TextureTypes.size()>
    Engine::make_descriptor_set_update_info_from_texture_set(TextureSetHandle handle, const TextureSet &set)
    {
        auto makeUpdateInfo = [handle](uint32_t binding, const auto &texture)-> DescriptorSetUpdateInfo
        {
            return {
                binding,
                handle.index,
                DescriptorSetImageInfo(
                    texture->getSampler(),
                    texture->getImageView(),
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    1
                )
            };
        };
        uint32_t textureIndex = 0;
        std::array updateInfos = {
            makeUpdateInfo(MainPass::s_MeshTextureBindStart + textureIndex++, set.albedo),
            makeUpdateInfo(MainPass::s_MeshTextureBindStart + textureIndex++, set.normal),
            makeUpdateInfo(MainPass::s_MeshTextureBindStart + textureIndex++, set.roughness),
            makeUpdateInfo(MainPass::s_MeshTextureBindStart + textureIndex++, set.metallic),
            makeUpdateInfo(MainPass::s_MeshTextureBindStart + textureIndex++, set.ao)
        };
        static_assert(TextureRegistry::s_TextureTypes.size() == updateInfos.size(),
                      "There is a missing texture in update info");
        return updateInfos;
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

    void Engine::render(const Window &window)
    {
        auto &frame = m_Frames[m_CurrentFrame];
        frame.reset(m_Context);

        readOutputBuffers(frame);

        auto acquired = m_Swapchain.acquire();
        if (!acquired)
        {
            m_Swapchain.recreate(window, m_Context, m_SampleCount);
            for (auto &f: m_Frames)
                f.recreateTextures(m_Context, m_Swapchain);
            createSceneTextureIds();
            return;
        }

        vk::Semaphore renderFinishedSemaphore = m_Swapchain.getPresentSemaphore(acquired->imageIndex); {
            CommandRecorder recorder(frame.getCommandBuffer());
            updateFrameBuffers(frame, recorder);

            executeCulling(frame, recorder);

            transitionForMainPass(frame, recorder);

            constexpr vk::ClearColorValue clearColor(std::array{0u, 0u, 0u, 0u});
            constexpr vk::ClearColorValue idClear(std::array{~0u, ~0u, ~0u, ~0u});

            std::array mainAndPickerAttachments{
                ColorAttachmentInfo(
                    m_Swapchain.getColorImageView(),
                    frame.getSceneTexture().getImageView(),
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    clearColor,
                    vk::ResolveModeFlagBits::eAverage
                ),
                ColorAttachmentInfo(
                    frame.getOutIdTexture().getImageView(),
                    frame.getResolvedOutIdTexture().getImageView(),
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eDontCare,
                    idClear,
                    vk::ResolveModeFlagBits::eSampleZero
                )
            };

            recorder.beginRendering(
                {
                    mainAndPickerAttachments,
                    frame.getExtent(),
                    m_Swapchain.getDepthImageView(),
                    vk::ImageLayout::eDepthAttachmentOptimal,
                    {}
                }
            );

            recorder.setViewport(frame.getExtent());
            recorder.setScissor(frame.getExtent());

            recordMeshData(frame, recorder);
            recordSkybox(frame, recorder);

            recorder.endRendering();

            transitionForOutlinePass(frame, recorder, acquired->imageIndex);


            std::array outlineAttachment{
                ColorAttachmentInfo(
                    frame.getSceneTexture().getImageView(),
                    {},
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eLoad,
                    vk::AttachmentStoreOp::eStore,
                    {}
                )
            };

            recorder.beginRendering(
                {
                    outlineAttachment,
                    frame.getExtent(),
                    {},
                    vk::ImageLayout::eUndefined,
                    {}
                });

            recordOutline(frame, recorder);
            recorder.endRendering();

            transitionForPickerAndPostProcess(frame, recorder);

            std::array imguiOverlay{
                ColorAttachmentInfo
                (m_Swapchain.getImageView(acquired->imageIndex),
                 {},
                 vk::ImageLayout::eColorAttachmentOptimal,
                 vk::AttachmentLoadOp::eLoad,
                 vk::AttachmentStoreOp::eStore,
                 vk::ClearColorValue{std::array{0.f, 0.f, 0.f, 1.f}}
                )
            };

            recorder.beginRendering({
                imguiOverlay,
                m_Swapchain.getExtent(),
                {},
                vk::ImageLayout::eUndefined,
                vk::RenderingFlagBits::eContentsSecondaryCommandBuffers
            });

            recordImGuiData(frame);
            recorder.getCommandBuffer().executeCommands(frame.getImGuiCommandBuffer());

            recorder.endRendering();

            transitionForPresent(recorder, acquired->imageIndex);
        }

        submit(m_Frames[m_CurrentFrame], acquired->imageAvailableSemaphore, renderFinishedSemaphore);

        if (!m_Swapchain.present(m_Context, acquired->imageIndex, renderFinishedSemaphore))
        {
            m_Swapchain.recreate(window, m_Context, m_SampleCount);
            for (auto &f: m_Frames)
                f.recreateTextures(m_Context, m_Swapchain);
            createSceneTextureIds();
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % s_FramesInFlight;
    }

    bool Engine::is_mesh_type_supported(std::string_view path)
    {
        static constexpr std::array<std::string_view, 3> supported =
        {
            "fbx",
            "gltf",
            "obj"
        };

        auto extension = path.substr(path.find_last_of(".") + 1);

        return std::ranges::contains(supported, extension);
    }

    bool Engine::is_image_type_supported(std::string_view path)
    {
        static constexpr std::array<std::string_view, 2> supported =
        {
            "jpeg",
            "png"
        };

        auto extension = path.substr(path.find_last_of(".") + 1);

        return std::ranges::contains(supported, extension);
    }

    bool Engine::isMeshCached(std::string_view path) const
    {
        return m_MeshCache.contains(std::string(path));
    }

    void Engine::saveScene(std::string_view folder) const
    {
        std::filesystem::path savePath = folder;
        savePath /= Scene::s_SaveFolder;
        if (!std::filesystem::exists(savePath))
            std::filesystem::create_directory(savePath);
        savePath /= std::string(m_Scene.getName().data()) + "." + s_SceneFileExtension.data();

        std::ofstream saveFile(savePath);
        if (saveFile.is_open())
            saveFile << m_Scene.serialize();
    }

    void Engine::loadScene(std::string_view path, int windowWidth, int windowHeight)
    {
        std::ifstream saveFile(path.data(), std::ios::ate | std::ios::binary);
        if (saveFile.is_open())
        {
            size_t fileSize = saveFile.tellg();
            std::string content;
            content.resize(fileSize);

            saveFile.seekg(0);
            saveFile.read(content.data(), fileSize);

            auto js = m_Scene.deserialize(content, windowWidth, windowHeight);
            if (js.contains("Mesh") && js["Mesh"].is_array())
            {
                for (const auto &meshJs: js["Mesh"])
                {
                    PendingMeshData pending;
                    pending.path = meshJs.value("path", "");
                    pending.name = meshJs.value("name", "");
                    pending.type = meshJs.value("type", MeshType::Unknown);

                    if (meshJs.contains("transform"))
                    {
                        auto &t = meshJs["transform"];
                        pending.transform.position = {t["position"][0], t["position"][1], t["position"][2]};
                        pending.transform.rotation = glm::quat(
                            t["rotation"][3],
                            t["rotation"][0],
                            t["rotation"][1],
                            t["rotation"][2]
                        );
                        pending.transform.scale = {t["scale"][0], t["scale"][1], t["scale"][2]};
                    }

                    if (meshJs.contains("material"))
                    {
                        auto &m = meshJs["material"];
                        pending.material.albedoAndRoughness = {
                            m["albedo"][0], m["albedo"][1], m["albedo"][2], m["roughness"]
                        };
                        pending.material.pbrParams = {
                            m["metallic"], m["ao"], 0.f, 0.f
                        };
                    }

                    if (!isMeshCached(pending.path))
                        if (auto loadData = MeshLoader::load(pending.path))
                            pending.data = std::move(*loadData);

                    m_PendingMeshData.push(std::move(pending));
                }
            }
        }
    }

    void Engine::setOnInfoLog(OnLog &&callback)
    {
        m_OnInfoLog = std::move(callback);
    }

    void Engine::setOnWarningLog(OnLog &&callback)
    {
        m_OnWarningLog = std::move(callback);
    }

    void Engine::setOnErrorLog(OnLog &&callback)
    {
        m_OnErrorLog = std::move(callback);
    }

    void Engine::setSceneViewportMousePos(uint32_t x, uint32_t y)
    {
        m_SceneViewportMousePos = {x, y};
    }

    ComputePicker &Engine::getPicker()
    {
        return m_ComputePicker;
    }

    uint32_t Engine::getPickedEntity() const
    {
        return m_PickedEntity;
    }

    OutlinePass &Engine::getOutlinePass()
    {
        return m_OutlinePass;
    }

    void Engine::cacheMesh(std::string_view path, MeshHandle meshHandle, TextureSetHandle materialHandle)
    {
        auto strPath = std::string(path);
        if (isMeshCached(path))
        {
            ++m_MeshCache[strPath].count;
            return;
        }
        m_MeshCache[strPath] = {meshHandle, materialHandle};
    }

    std::optional<Engine::MeshCache> Engine::uncacheMesh(std::string_view path)
    {
        auto it = m_MeshCache.find(std::string(path));
        if (it == m_MeshCache.end())
            return std::nullopt;
        auto &count = it->second.count;
        if (count > 1)
        {
            --count;
            return std::nullopt;
        }
        std::optional cache = it->second;
        m_MeshCache.erase(it);
        return cache;
    }

    void Engine::executeCulling(const FrameData &frame, const CommandRecorder &recorder)
    {
        const auto cmd = recorder.getCommandBuffer();
        auto totalObjects = static_cast<uint32_t>(m_Scene.getEntityRegistry().view<MeshComponent>().size());
        if (totalObjects == 0)
            return;

        recorder.getCommandBuffer().fillBuffer(
            frame.getCullerCountBuffer().getBuffer(),
            0,
            frame.getCullerCountBuffer().getSize(),
            0
        );

        std::array countBufferBarrier = {frame.getCullerCountBufferFillMemoryBarrier()};
        recorder.bufferMemoryBarriers(countBufferBarrier);

        m_ComputeCuller.bind(cmd);
        frame.getCullerDescriptorSet().bind(m_ComputeCuller.getPipeline(), cmd, vk::PipelineBindPoint::eCompute);

        const auto &camera = m_Scene.getEntityRegistry().get<CameraData>(m_Scene.getMainCamera());
        auto planes = Camera::get_frustum_planes(camera.projection, camera.view);
        m_ComputeCuller.push<ComputePassesPushConstants::CameraFrustum>(cmd, planes, totalObjects);

        uint32_t groupX = (totalObjects + 255) / 256;
        m_ComputeCuller.execute(cmd, {groupX, 1, 1});

        recorder.bufferMemoryBarriers(frame.getCullerBufferMemoryBarriers());
    }

    void Engine::transitionForMainPass(const FrameData &frame, const CommandRecorder &recorder) const
    {
        recorder.imageBarrier({
            m_Swapchain.getColorImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal
        });

        recorder.imageBarrier({
            frame.getSceneTexture().getImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eShaderRead,
            vk::AccessFlagBits2::eColorAttachmentWrite
        });

        recorder.imageBarrier({
            m_Swapchain.getDepthImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests,
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::ImageAspectFlagBits::eDepth
        });

        recorder.imageBarrier({
            frame.getOutIdTexture().getImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eAllGraphics,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eColorAttachmentWrite
        });

        recorder.imageBarrier({
            frame.getResolvedOutIdTexture().getImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eAllGraphics,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eColorAttachmentWrite
        });
    }

    void Engine::transitionForOutlinePass(const FrameData &frame, const CommandRecorder &recorder,
                                          uint32_t imageIndex) const
    {
        recorder.imageBarrier({
            m_Swapchain.getImage(imageIndex),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal
        });

        recorder.imageBarrier({
            frame.getResolvedOutIdTexture().getImage(),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eShaderRead
        });
    }

    void Engine::transitionForPickerAndPostProcess(const FrameData &frame, const CommandRecorder &recorder) const
    {
        recorder.imageBarrier({
            frame.getSceneTexture().getImage(),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eShaderRead
        });

        recorder.imageBarrier({
            frame.getResolvedOutIdTexture().getImage(),
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eGeneral,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eShaderRead,
            vk::AccessFlagBits2::eShaderRead
        });

        recordPicker(frame, recorder);
        std::array pickerMemBarrier{frame.getPickerBufferMemoryBarrier()};
        recorder.bufferMemoryBarriers(pickerMemBarrier);

        recorder.imageBarrier({
            frame.getResolvedOutIdTexture().getImage(),
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eShaderRead,
            vk::AccessFlagBits2::eColorAttachmentWrite
        });
    }

    void Engine::transitionForPresent(const CommandRecorder &recorder, uint32_t imageIndex) const
    {
        recorder.imageBarrier({
            m_Swapchain.getImage(imageIndex),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eBottomOfPipe,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eNone
        });
    }

    void Engine::recordMeshData(const FrameData &frame, const CommandRecorder &recorder) const
    {
        const auto cmd = recorder.getCommandBuffer();
        m_MainPass.bind(cmd);
        m_MeshRegistry.bind(recorder.getCommandBuffer());
        frame.getDescriptorSet().bind(m_MainPass.getPipeline(), cmd);

        recorder.drawIndexedIndirectCount(
            frame.getIndirectBuffer(),
            frame.getCullerCountBuffer(),
            MainPass::s_MaxMeshCount
        );
    }

    void Engine::recordSkybox(const FrameData &frame, const CommandRecorder &recorder) const
    {
        const auto cmd = recorder.getCommandBuffer();
        m_SkyboxPass.bind(cmd);
        frame.getSkyboxDescriptorSet().bind(m_SkyboxPass.getPipeline(), cmd);
        auto cubeView = m_MeshRegistry.view(m_MeshRegistry.getBuiltins().cube);
        cmd.drawIndexed(
            cubeView.indexCount,
            1,
            cubeView.firstIndex,
            cubeView.vertexOffset,
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
            vk::Format::eUndefined,
            vk::Format::eUndefined,
            vk::SampleCountFlagBits::e1
        );

        m_ImGuiBackend.beginFrame();
        m_OnEditorRender(m_Scene);
        m_ImGuiBackend.endFrame();

        CommandRecorder recorder(frame.getImGuiCommandBuffer(), inheritanceInfo);
        m_ImGuiBackend.recordDrawData(recorder.getCommandBuffer());
    }

    void Engine::recordPicker(const FrameData &frame, const CommandRecorder &recorder) const
    {
        const auto cmd = recorder.getCommandBuffer();
        m_ComputePicker.bind(cmd);
        frame.getPickerDescriptorSet().bind(m_ComputePicker.getPipeline(), cmd,
                                            vk::PipelineBindPoint::eCompute);
        m_ComputePicker.push<ComputePassesPushConstants::MouseCords>(cmd, m_SceneViewportMousePos.x, m_SceneViewportMousePos.y);
        m_ComputePicker.execute(
            cmd,
            {1, 1, 1}
        );
    }

    void Engine::recordOutline(const FrameData &frame, const CommandRecorder &recorder) const
    {
        const auto cmd = recorder.getCommandBuffer();
        m_OutlinePass.bind(cmd);
        frame.getOutlineDescriptorSet().bind(m_OutlinePass.getPipeline(), cmd);
        m_OutlinePass.push(cmd);
        cmd.draw(3, 1, 0, 0);
    }

    void Engine::update(float deltaTime, Window &window)
    {
        handleEvent(window);

        pollPendingData();
        updatePendingFrameTasks();

        auto view = m_Scene.getEntityRegistry().view<CameraComponent, CameraData>();
        for (auto entity: view)
        {
            auto &camera = view.get<CameraComponent>(entity);
            Camera::update_movement(camera, window, deltaTime);
            Camera::update_look_at(camera, window, deltaTime);
            auto &data = view.get<CameraData>(entity);
            data.view = Camera::get_view(camera);
            data.positionAndExposure = {camera.position, camera.exposure};
        }
    }

    void Engine::updateFrameBuffers(FrameData &frame, const CommandRecorder &recorder)
    {
        updateCameraBuffer(frame);
        updateMeshDataBuffer(frame);
        updateSceneBuffer(frame);
        updateCullerBuffers(frame, recorder);

        recorder.bufferMemoryBarriers(frame.getBufferMemoryBarriers());
    }

    void Engine::updateCameraBuffer(FrameData &frame) const
    {
        const auto &data = m_Scene.getEntityRegistry().get<CameraData>(m_Scene.getMainCamera());
        frame.getCameraBuffer().upload(
            &data,
            sizeof(CameraData)
        );
    }

    void Engine::updateMeshDataBuffer(FrameData &frame) const
    {
        std::vector<MeshData> data;
        auto view = m_Scene.getEntityRegistry().view<MeshTransformData, MeshMaterialData, MeshComponent,
            MaterialComponent>();
        data.reserve(view.size_hint());
        for (auto entity: view)
        {
            const auto &transform = view.get<MeshTransformData>(entity);
            auto boundingSphere = view.get<MeshComponent>(entity).boundingSphere;
            auto material = view.get<MeshMaterialData>(entity);
            material.materialIdx = view.get<MaterialComponent>(entity).handle.index;
            data.emplace_back(
                transform.getModelMatrix(),
                boundingSphere,
                material,
                static_cast<uint32_t>(entity)
            );
        }
        frame.getModelBuffer().upload(data.data(), data.size() * sizeof(MeshData));
    }

    void Engine::updateSceneBuffer(FrameData &frame) const
    {
        const auto &data = m_Scene.getData();
        frame.getSceneBuffer().upload(&data, sizeof(SceneData));
    }

    void Engine::updateCullerBuffers(const FrameData &frame, const CommandRecorder &recorder)
    {
        std::vector<vk::DrawIndexedIndirectCommand> indirectCommands;
        auto view = m_Scene.getEntityRegistry().view<MeshComponent>();
        indirectCommands.reserve(view.size());
        view.each([this, &indirectCommands](const auto &mesh)
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
        if (indirectCommands.empty())
            return;

        frame.getCullerInputCommandsBuffer().upload(indirectCommands.data(),
                                                    indirectCommands.size() * sizeof(vk::DrawIndexedIndirectCommand));
    }

    void Engine::readOutputBuffers(const FrameData &frame)
    {
        m_PickedEntity = frame.getPickerBuffer().read<uint32_t>();
    }

    void Engine::handleEvent(Window &window)
    {
        if (auto event = window.getEvent())
            std::visit(
                VisitOverloads
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
                                auto &focused = view.get<CameraComponent>(entity).focused;
                                focused = !focused;
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
        if (auto data = m_PendingMeshData.tryPop())
        {
            if (data->type == MeshType::Unknown)
                return;
            if (data->type != MeshType::Loaded)
            {
                auto createMeshEntity = [this, &data](auto meshHandle, const auto &vertices)
                {
                    const auto &material = m_TextureRegistry.view(m_TextureRegistry.getDefaultSetHandle());
                    auto textureHandle = m_TextureRegistry.registerTextureSet(material);
                    m_Scene.createMeshEntity(
                        data->name.empty() ? m_Scene.getMeshEntityName() : data->name,
                        {
                            meshHandle,
                            data->path,
                            data->type,
                            Geometry::computeBoundingSphere(vertices)
                        },
                        textureHandle,
                        data->transform,
                        data->material
                    );
                };
                switch (data->type)
                {
                    case MeshType::Cube:
                        createMeshEntity(
                            m_MeshRegistry.getBuiltins().cube,
                            MeshRegistry::generate_cube().vertices
                            );
                        break;
                    case MeshType::Sphere:
                        createMeshEntity(
                            m_MeshRegistry.getBuiltins().sphere,
                            MeshRegistry::generate_sphere().vertices
                            );
                        break;
                    default:
                        break;
                }
                return;
            }

            const auto &meshData = data->data;
            MeshHandle meshHandle;
            TextureSetHandle textureHandle;
            if (isMeshCached(data->path))
            {
                auto cache = m_MeshCache.at(data->path);
                const auto &material = m_TextureRegistry.view(cache.materialHandle);
                auto newHandle = m_TextureRegistry.registerTextureSet(material);

                auto updateInfos = make_descriptor_set_update_info_from_texture_set(newHandle, material);

                for (const auto &frame: m_Frames)
                    frame.getDescriptorSet().updateInfo(
                        m_Context,
                        updateInfos
                    );
                meshHandle = cache.meshHandle;
                textureHandle = cache.materialHandle;
            } else
            {
                meshHandle = uploadMeshDataToRegistry(meshData.meshData);
                textureHandle = uploadMaterialDataToRegistry(meshData.materialData);
            }
            cacheMesh(data->path, meshHandle, textureHandle);
            m_Scene.createMeshEntity(data->name.empty() ? m_Scene.getMeshEntityName() : data->name,
                                     {
                                         meshHandle,
                                         data->path,
                                         data->type,
                                         data->data.boundingSphere
                                     },
                                     textureHandle,
                                     data->transform,
                                     data->material
            );
            m_OnInfoLog(std::format("Loaded {} successfully", data->path));
        }
    }

    MeshHandle Engine::uploadMeshDataToRegistry(const MeshRegistry::MeshData &data)
    {
        std::vector<Buffer> stagingBuffers;
        auto otc = OneTimeCommand::create(m_Context);
        auto handle = m_MeshRegistry.upload(m_Context, otc.getCommandBuffer(), data, stagingBuffers);
        otc.submit(m_Context);
        return handle;
    }

    TextureSetHandle Engine::uploadMaterialDataToRegistry(const TextureRegistry::MaterialData &data)
    {
        auto set = m_TextureRegistry.createSetFromMaterialData(m_Context, data);
        auto handle = m_TextureRegistry.registerTextureSet(set);

        auto updateInfos = make_descriptor_set_update_info_from_texture_set(handle, set);

        for (const auto &frame: m_Frames)
            frame.getDescriptorSet().updateInfo(
                m_Context,
                updateInfos
            );

        return handle;
    }

    void Engine::updatePendingFrameTasks()
    {
        std::erase_if(m_PendingFrameTasks, [](auto &task)
        {
            --task.remainingFrames;
            if (task.remainingFrames == 0)
            {
                task.task();
                return true;
            }
            return false;
        });
    }
}
