#include "Engine.h"

#include <fstream>
#include <magic_enum/magic_enum.hpp>

#include "FileDialog.h"
#include "Geometry.h"
#include "command/CommandRecorder.h"
#include "Log.h"
#include "command/OneTimeCommand.h"
#include "components/entt/CachedPhysicsData.h"
#include "components/entt/CameraComponent.h"
#include "components/entt/HierarchyComponent.h"
#include "components/entt/MaterialComponent.h"
#include "components/entt/MeshComponent.h"
#include "components/entt/PendingUploadComponent.h"
#include "components/entt/PhysicsComponent.h"
#include "components/entt/PhysicsControlComponent.h"
#include "components/entt/TagComponent.h"
#include "components/gpu/CameraData.h"
#include "components/gpu/MeshData.h"
#include "components/gpu/MeshTransformData.h"
#include "components/gpu/TransformComponent.h"
#include "texture/TextureAllocator.h"

namespace kailux
{
    Engine::Engine() : mSampleCount(vk::SampleCountFlagBits::e1),
                       mAssetPipeline(mContext, mMeshRegistry, mTextureRegistry, mTransferManager, mScene,mFrames),
                       mPhysicsSystem(mScene, mPhysicsRegistry),
                       mCurrentFrame(0),
                       mPickedEntity(~0u)
    {
    }

    Engine::Engine(Engine &&other) noexcept : mContext(std::move(other.mContext)),
                                              mSampleCount(other.mSampleCount),
                                              mSwapchain(std::move(other.mSwapchain)),
                                              mImGuiBackend(std::move(other.mImGuiBackend)),
                                              mTransferManager(std::move(other.mTransferManager)),
                                              mMeshRegistry(std::move(other.mMeshRegistry)),
                                              mTextureRegistry(std::move(other.mTextureRegistry)),
                                              mPhysicsRegistry(std::move(other.mPhysicsRegistry)),
                                              mGizmoRegistry(std::move(other.mGizmoRegistry)),
                                              mAssetPipeline(std::move(other.mAssetPipeline)),
                                              mPhysicsSystem(std::move(other.mPhysicsSystem)),
                                              mDeferredResourceEraser(std::move(other.mDeferredResourceEraser)),
                                              mFrames(std::move(other.mFrames)),
                                              mCurrentFrame(other.mCurrentFrame),
                                              mSceneTextureIds(other.mSceneTextureIds),
                                              mScene(std::move(other.mScene)),
                                              mMainPass(std::move(other.mMainPass)),
                                              mSkyboxPass(std::move(other.mSkyboxPass)),
                                              mGizmoPass(std::move(other.mGizmoPass)),
                                              mOutlinePass(std::move(other.mOutlinePass)),
                                              mComputePicker(std::move(other.mComputePicker)),
                                              mPickedEntity(other.mPickedEntity),
                                              mComputeCuller(std::move(other.mComputeCuller)),
                                              mOnInfoLog(std::move(other.mOnInfoLog)),
                                              mOnWarningLog(std::move(other.mOnWarningLog)),
                                              mOnErrorLog(std::move(other.mOnErrorLog))
    {
        createAssetPipeline();
        createPhysicsSystem();
    }

    Engine &Engine::operator=(Engine &&other) noexcept
    {
        if (this != &other)
        {
            mContext = std::move(other.mContext);
            mSampleCount = other.mSampleCount;
            mSwapchain = std::move(other.mSwapchain);
            mImGuiBackend = std::move(other.mImGuiBackend);
            mTransferManager = std::move(other.mTransferManager);
            mMeshRegistry = std::move(other.mMeshRegistry);
            mTextureRegistry = std::move(other.mTextureRegistry);
            mPhysicsRegistry = std::move(other.mPhysicsRegistry);
            mGizmoRegistry = std::move(other.mGizmoRegistry);
            mAssetPipeline = std::move(other.mAssetPipeline);
            mPhysicsSystem = std::move(other.mPhysicsSystem);
            mDeferredResourceEraser = std::move(other.mDeferredResourceEraser);
            mFrames = std::move(other.mFrames);
            mCurrentFrame = other.mCurrentFrame;
            mSceneTextureIds = other.mSceneTextureIds;
            mScene = std::move(other.mScene);
            mMainPass = std::move(other.mMainPass);
            mSkyboxPass = std::move(other.mSkyboxPass);
            mGizmoPass = std::move(other.mGizmoPass);
            mOutlinePass = std::move(other.mOutlinePass);
            mComputePicker = std::move(other.mComputePicker);
            mPickedEntity = other.mPickedEntity;
            mComputeCuller = std::move(other.mComputeCuller);
            mOnInfoLog = std::move(other.mOnInfoLog);
            mOnWarningLog = std::move(other.mOnWarningLog);
            mOnErrorLog = std::move(other.mOnErrorLog);

            createAssetPipeline();
            createPhysicsSystem();
        }
        return *this;
    }

    Engine::~Engine()
    {
        if (mContext.getDevice())
        {
            waitIdle();
            mTransferManager.clear();
            mFrames = {};
            OneTimeCommand::destroy_command_pools();
        }
    }

    Engine Engine::create(Window &window)
    {
        Engine engine;
        engine.createRenderingContext(window);
        OneTimeCommand::create_command_pools(engine.mContext);
        engine.createMainPass();
        engine.createSkybox();
        engine.createGizmoPass();
        engine.createOutlinePass();
        engine.createTransferManager();
        engine.createMeshRegistry();
        engine.createTextureRegistry();
        engine.createPhysicsRegistry();
        engine.createGizmoRegistry();
        engine.createComputePicker();
        engine.createComputeCuller();
        engine.createFrameResources();
        engine.createImGui(window);
        engine.createScene();
        engine.createSceneTextureIds();
        engine.createSceneEntities(window);
        engine.createAssetPipeline();
        engine.createPhysicsSystem();
        return engine;
    }

    void Engine::setOnEditorRender(OnEditorRender &&callback)
    {
        mOnEditorRender = std::move(callback);
    }

    void Engine::waitIdle() const
    {
        mContext.getDevice().waitIdle();
    }

    Queue<AssetPipeline::PendingMeshData> &Engine::getPendingMeshDataQueue()
    {
        return mAssetPipeline.getPendingQueue();
    }

    void Engine::unregisterMesh(MeshHandle handle, std::string_view path)
    {
        if (mAssetPipeline.uncache(path))
            mMeshRegistry.destroy(handle);
    }

    void Engine::unregisterTextureSet(TextureSetHandle handle)
    {
        auto defaultHandle = mTextureRegistry.getDefaultSetHandle();
        const auto &defaultSet = mTextureRegistry.view(defaultHandle);
        auto updateInfos = make_descriptor_set_update_info_from_texture_set(handle, defaultSet);

        for (const auto &frame: mFrames)
            frame.getDescriptorSet().updateInfo(
                mContext,
                updateInfos
            );

        mDeferredResourceEraser.enqueue([this, handle]()
        {
            mTextureRegistry.unregisterTextureSet(handle);
        });
    }

    ImTextureID Engine::getAssetBrowserDirectoryTextureId() const
    {
        return ImGuiBackend::get_texture_id_from_texture(mTextureRegistry.getAssetBrowserDirectoryIconTexture());
    }

    ImTextureID Engine::getAssetBrowserFileTextureId() const
    {
        return ImGuiBackend::get_texture_id_from_texture(mTextureRegistry.getAssetBrowserFileIconTexture());
    }

    ImTextureID Engine::getSceneTextureId() const
    {
        return mSceneTextureIds[mCurrentFrame];
    }

    void Engine::createRenderingContext(Window &window)
    {
        mContext = Context::create(window);
        mSampleCount = mContext.getMaxUsableSampleCount();
        mSwapchain = Swapchain::create(window, mContext, mSampleCount);
    }

    void Engine::createMainPass()
    {
        mMainPass = MainPass::create(
            mContext,
            mSwapchain,
            details::kFramesInFlight
        );
    }

    void Engine::createSkybox()
    {
        mSkyboxPass = SkyboxPass::create(
            mContext,
            mSwapchain,
            details::kFramesInFlight
        );
    }

    void Engine::createGizmoPass()
    {
        mGizmoPass = GizmoPass::create(mContext, mSwapchain, details::kFramesInFlight);
    }

    void Engine::createOutlinePass()
    {
        mOutlinePass = OutlinePass::create(
            mContext,
            mSwapchain,
            details::kFramesInFlight
        );
    }

    void Engine::createFrameResources()
    {
        for (auto &frame: mFrames)
            frame = FrameData::create(
                mContext,
                mSwapchain,
                mMainPass,
                mSkyboxPass,
                mGizmoPass,
                mComputePicker,
                mOutlinePass,
                mComputeCuller, mTextureRegistry
            );
    }

    void Engine::createTransferManager()
    {
        mTransferManager = TransferManager::create();
    }

    void Engine::createMeshRegistry()
    {
        std::vector<Buffer> stagingBuffers;
        auto otc = OneTimeCommand::create(mContext);
        mMeshRegistry = MeshRegistry::create(mContext, otc.getCommandBuffer(), stagingBuffers);
        otc.submit(mContext);
    }

    void Engine::createTextureRegistry()
    {
        mTextureRegistry = TextureRegistry::create(
            mContext,
            details::kMaxMeshes,
            kDirectoryIconPath,
            kFileIconPath
        );
    }

    void Engine::createPhysicsRegistry()
    {
        mPhysicsRegistry = PhysicsRegistry::create();
    }

    void Engine::createGizmoRegistry()
    {
        std::vector<Buffer> stagingBuffers;
        auto otc = OneTimeCommand::create(mContext);
        mGizmoRegistry = GizmoRegistry::create(mContext, otc.getCommandBuffer(), stagingBuffers);
        otc.submit(mContext);
    }

    void Engine::createAssetPipeline()
    {
        mAssetPipeline = AssetPipeline(mContext, mMeshRegistry, mTextureRegistry, mTransferManager, mScene, mFrames);
        mAssetPipeline.setOnInfoLog([this](auto msg)
        {
            mOnInfoLog(msg);
        });
        mAssetPipeline.setOnWarningLog([this](auto msg)
        {
            mOnWarningLog(msg);
        });
    }

    void Engine::createPhysicsSystem()
    {
        mPhysicsSystem = PhysicsSystem(mScene, mPhysicsRegistry);
        mPhysicsSystem.setOnWarningLog([this](auto msg)
        {
            mOnWarningLog(msg);
        });
    }

    void Engine::createImGui(Window &window)
    {
        mImGuiBackend = ImGuiBackend::create(window, mContext, mSwapchain, mSampleCount);
    }

    void Engine::createSceneTextureIds()
    {
        for (uint32_t i = 0; i < details::kFramesInFlight; i++)
            mSceneTextureIds[i] = ImGuiBackend::get_texture_id_from_texture(mFrames[i].getSceneTexture());
    }

    void Engine::createComputePicker()
    {
        mComputePicker = ComputePicker::create(mContext, details::kFramesInFlight);
    }

    void Engine::createComputeCuller()
    {
        mComputeCuller = ComputeCuller::create(mContext, details::kFramesInFlight);
    }

    void Engine::createScene()
    {
        mScene = Scene::create("MainScene");
    }

    void Engine::createSceneEntities(const Window &window)
    {
        int windowWidth, windowHeight;
        window.getFramebufferSize(windowWidth, windowHeight);
        auto cameraEntity = mScene.createCameraEntity(
            "MainCamera",
            true,
            windowWidth,
            windowHeight
        );
        mScene.setMainCamera(cameraEntity);
    }

    std::array<DescriptorSetUpdateInfo, TextureRegistry::kTextureTypes.size()>
    Engine::make_descriptor_set_update_info_from_texture_set(TextureSetHandle slotToOverwrite,
                                                             const TextureSet &replacementSet)
    {
        auto makeUpdateInfo = [slotToOverwrite](uint32_t binding, const auto &texture)-> DescriptorSetUpdateInfo
        {
            return {
                binding,
                slotToOverwrite.index,
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
            makeUpdateInfo(MainPass::kMeshTextureBindStart + textureIndex++, replacementSet.albedo),
            makeUpdateInfo(MainPass::kMeshTextureBindStart + textureIndex++, replacementSet.normal),
            makeUpdateInfo(MainPass::kMeshTextureBindStart + textureIndex++, replacementSet.roughness),
            makeUpdateInfo(MainPass::kMeshTextureBindStart + textureIndex++, replacementSet.metallic),
            makeUpdateInfo(MainPass::kMeshTextureBindStart + textureIndex++, replacementSet.ao)
        };
        static_assert(TextureRegistry::kTextureTypes.size() == updateInfos.size(),
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

        mContext.getGraphicsQueue().submit2(submitInfo, frame.getFenceInFlight());
    }

    void Engine::render(const Window &window)
    {
        auto &frame = mFrames[mCurrentFrame];
        frame.reset(mContext);

        readOutputBuffers(frame);

        auto acquired = mSwapchain.acquire();
        if (!acquired)
        {
            mSwapchain.recreate(window, mContext, mSampleCount);
            for (auto &f: mFrames)
                f.recreateTextures(mContext, mSwapchain);
            createSceneTextureIds();
            return;
        }

        vk::Semaphore renderFinishedSemaphore = mSwapchain.getPresentSemaphore(acquired->imageIndex); {
            CommandRecorder recorder(frame.getCommandBuffer());
            updateFrameBuffers(frame, recorder);

            executeCulling(frame, recorder);

            transitionForMainPass(frame, recorder);

            constexpr vk::ClearColorValue clearColor(std::array{0u, 0u, 0u, 0u});
            constexpr vk::ClearColorValue idClear(std::array{~0u, ~0u, ~0u, ~0u});

            std::array mainAndPickerAttachments{
                ColorAttachmentInfo(
                    mSwapchain.getColorImageView(),
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
                    mSwapchain.getDepthImageView(),
                    vk::ImageLayout::eDepthAttachmentOptimal,
                    {}
                }
            );

            recorder.setViewport(frame.getExtent());
            recorder.setScissor(frame.getExtent());

            recordMeshData(frame, recorder);
            recordSkybox(frame, recorder);
            recordGizmos(frame, recorder);

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
                (mSwapchain.getImageView(acquired->imageIndex),
                 {},
                 vk::ImageLayout::eColorAttachmentOptimal,
                 vk::AttachmentLoadOp::eLoad,
                 vk::AttachmentStoreOp::eStore,
                 vk::ClearColorValue{std::array{0.f, 0.f, 0.f, 1.f}}
                )
            };

            recorder.beginRendering({
                imguiOverlay,
                mSwapchain.getExtent(),
                {},
                vk::ImageLayout::eUndefined,
                vk::RenderingFlagBits::eContentsSecondaryCommandBuffers
            });

            recordImGuiData(frame);
            recorder.getCommandBuffer().executeCommands(frame.getImGuiCommandBuffer());

            recorder.endRendering();

            transitionForPresent(recorder, acquired->imageIndex);
        }

        submit(mFrames[mCurrentFrame], acquired->imageAvailableSemaphore, renderFinishedSemaphore);

        if (!mSwapchain.present(mContext, acquired->imageIndex, renderFinishedSemaphore))
        {
            mSwapchain.recreate(window, mContext, mSampleCount);
            for (auto &f: mFrames)
                f.recreateTextures(mContext, mSwapchain);
            createSceneTextureIds();
        }

        mCurrentFrame = (mCurrentFrame + 1) % details::kFramesInFlight;
    }

    bool Engine::is_mesh_type_supported(std::string_view path)
    {
        static constexpr std::array<std::string_view, 3> supported =
        {
            "fbx",
            "gltf",
            "obj"
        };

        auto extension = path.substr(path.find_last_of('.') + 1);

        return std::ranges::contains(supported, extension);
    }

    bool Engine::is_image_type_supported(std::string_view path)
    {
        static constexpr std::array<std::string_view, 2> supported =
        {
            "jpeg",
            "png"
        };

        auto extension = path.substr(path.find_last_of('.') + 1);

        return std::ranges::contains(supported, extension);
    }

    bool Engine::isMeshCached(std::string_view path) const
    {
        return mAssetPipeline.isCached(path);
    }

    void Engine::saveScene(std::string_view folder) const
    {
        std::filesystem::path savePath = folder;
        savePath /= Scene::kSaveFolder;
        if (!std::filesystem::exists(savePath))
            std::filesystem::create_directory(savePath);
        savePath /= std::string(mScene.getName().data()) + "." + kSceneFileExtension.data();

        std::ofstream saveFile(savePath);
        if (saveFile.is_open())
            saveFile << mScene.serialize();
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

            auto js = mScene.deserialize(content, windowWidth, windowHeight);
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

                    if (meshJs.contains("physics"))
                    {
                        auto &p = meshJs["physics"];
                        pending.bodyType = static_cast<PhysicsBodyType>(p["body"]);
                    }

                    if (!isMeshCached(pending.path))
                        if (auto loadData = MeshLoader::load(pending.path))
                            pending.data = std::move(*loadData);
                }
            }
        }
    }

    void Engine::setOnInfoLog(OnLog &&callback)
    {
        mOnInfoLog = std::move(callback);
        mAssetPipeline.setOnInfoLog([this](auto msg)
        {
            mOnInfoLog(msg);
        });
    }

    void Engine::setOnWarningLog(OnLog &&callback)
    {
        mOnWarningLog = std::move(callback);
    }

    void Engine::setOnErrorLog(OnLog &&callback)
    {
        mOnErrorLog = std::move(callback);
    }

    void Engine::setSceneViewportMousePos(uint32_t x, uint32_t y)
    {
        mSceneViewportMousePos = {x, y};
    }

    void Engine::setOutlineInfo(glm::vec3 color, uint32_t entity)
    {
        mOutlineInfo = {{color, 1.f}, entity};
    }

    uint32_t Engine::getPickedEntity() const
    {
        return mPickedEntity;
    }

    void Engine::updateBodyType(BodyHandle handle, PhysicsBodyType type)
    {
        mPhysicsRegistry.setBodyType(handle, type);
    }

    void Engine::updateBodyScale(BodyHandle handle, const glm::vec3 &scale)
    {
        mPhysicsRegistry.updateBodyScale(handle, scale);
    }

    void Engine::setSimulationState(SimulationState state)
    {
        mPhysicsSystem.setSimulationState(state);
    }

    void Engine::executeCulling(const FrameData &frame, const CommandRecorder &recorder)
    {
        const auto cmd = recorder.getCommandBuffer();

        recorder.getCommandBuffer().fillBuffer(
            frame.getCullerCountBuffer().getBuffer(),
            0,
            frame.getCullerCountBuffer().getSize(),
            0
        );

        std::array countBufferBarrier = {frame.getCullerCountBufferFillMemoryBarrier()};
        recorder.bufferMemoryBarriers(countBufferBarrier);

        auto totalObjects = static_cast<uint32_t>(
            mScene.getEntityRegistry().view<MeshComponent>(entt::exclude<PendingUploadComponent>).size_hint());
        if (totalObjects == 0)
            return;

        mComputeCuller.bind(cmd);
        frame.getCullerDescriptorSet().bind(mComputeCuller.getPipeline(), cmd, vk::PipelineBindPoint::eCompute);

        const auto &camera = mScene.getEntityRegistry().get<CameraData>(mScene.getMainCamera());
        auto planes = Camera::get_frustum_planes(camera.projection, camera.view);
        mComputeCuller.push<ComputePassesPushConstants::CameraFrustum>(cmd, {planes, totalObjects});

        uint32_t groupX = (totalObjects + 255) / 256;
        mComputeCuller.execute(cmd, {groupX, 1, 1});

        recorder.bufferMemoryBarriers(frame.getCullerBufferMemoryBarriers());
    }

    void Engine::transitionForMainPass(const FrameData &frame, const CommandRecorder &recorder) const
    {
        recorder.imageBarrier({
            mSwapchain.getColorImage(),
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
            mSwapchain.getDepthImage(),
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
            mSwapchain.getImage(imageIndex),
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
            mSwapchain.getImage(imageIndex),
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
        mMainPass.bind(cmd);
        mMeshRegistry.bind(recorder.getCommandBuffer());
        frame.getDescriptorSet().bind(mMainPass.getPipeline(), cmd);

        recorder.drawIndexedIndirectCount(
            frame.getIndirectBuffer(),
            frame.getCullerCountBuffer(),
            details::kMaxMeshes
        );
    }

    void Engine::recordSkybox(const FrameData &frame, const CommandRecorder &recorder) const
    {
        const auto cmd = recorder.getCommandBuffer();
        mSkyboxPass.bind(cmd);
        frame.getSkyboxDescriptorSet().bind(mSkyboxPass.getPipeline(), cmd);
        auto cubeView = mMeshRegistry.view(mMeshRegistry.getBuiltins().cube);
        cmd.drawIndexed(
            cubeView.indexCount,
            1,
            cubeView.firstIndex,
            cubeView.vertexOffset,
            0
        );
    }

    void Engine::recordGizmos(const FrameData &frame, const CommandRecorder &recorder) const
    {
        if (mPhysicsSystem.getSimulationState() == SimulationState::Running)
            return;

        const auto cmd = recorder.getCommandBuffer();
        mGizmoPass.bind(cmd);
        mGizmoRegistry.bind(cmd);
        frame.getGizmoDescriptorSet().bind(mGizmoPass.getPipeline(), cmd);
        auto view = mScene.getEntityRegistry().view<GizmoComponent, TransformComponent>();
        view.each([&](const auto& component, const auto& transform)
        {
            auto gizmoView = mGizmoRegistry.view(component.handle);

            GraphicsPassesPushConstants::Gizmo pc{
                glm::vec4(transform.transform.position, component.scale),
                component.color
            };

            mGizmoPass.push(cmd, pc);

            cmd.drawIndexed(
                gizmoView.indexCount,
                1,
                gizmoView.firstIndex,
                gizmoView.vertexOffset,
                0
                );
        });
    }

    void Engine::recordImGuiData(const FrameData &frame)
    {
        auto format = mSwapchain.getFormat();
        auto inheritanceInfo = vk::CommandBufferInheritanceRenderingInfo(
            {},
            {},
            1,
            &format,
            vk::Format::eUndefined,
            vk::Format::eUndefined,
            vk::SampleCountFlagBits::e1
        );

        mImGuiBackend.beginFrame();
        mOnEditorRender(mScene);
        mImGuiBackend.endFrame();

        CommandRecorder recorder(frame.getImGuiCommandBuffer(), inheritanceInfo);
        mImGuiBackend.recordDrawData(recorder.getCommandBuffer());
    }

    void Engine::recordPicker(const FrameData &frame, const CommandRecorder &recorder) const
    {
        const auto cmd = recorder.getCommandBuffer();
        mComputePicker.bind(cmd);
        frame.getPickerDescriptorSet().bind(mComputePicker.getPipeline(), cmd,
                                            vk::PipelineBindPoint::eCompute);
        mComputePicker.push(cmd, mSceneViewportMousePos);
        mComputePicker.execute(
            cmd,
            {1, 1, 1}
        );
    }

    void Engine::recordOutline(const FrameData &frame, const CommandRecorder &recorder) const
    {
        const auto cmd = recorder.getCommandBuffer();
        mOutlinePass.bind(cmd);
        frame.getOutlineDescriptorSet().bind(mOutlinePass.getPipeline(), cmd);
        mOutlinePass.push(cmd, mOutlineInfo);
        cmd.draw(3, 1, 0, 0);
    }

    void Engine::update(float deltaTime, Window &window)
    {
        handleEvent(window);

        mAssetPipeline.poll();
        mTransferManager.poll(mContext);
        mDeferredResourceEraser.tick();

        if (mPhysicsSystem.getSimulationState() == SimulationState::Running)
            mPhysicsSystem.update(deltaTime);

        mScene.update();

        auto view = mScene.getEntityRegistry().view<CameraComponent, CameraData>();
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
        const auto &data = mScene.getEntityRegistry().get<CameraData>(mScene.getMainCamera());
        frame.getCameraBuffer().upload(
            &data,
            sizeof(CameraData)
        );
    }

    void Engine::updateMeshDataBuffer(FrameData &frame) const
    {
        std::vector<MeshData> data;
        auto view = mScene.getEntityRegistry().view<
            TransformComponent,
            MeshMaterialData,
            MeshComponent,
            MaterialComponent>
        (entt::exclude<PendingUploadComponent>);
        data.reserve(view.size_hint());
        for (auto entity: view)
        {
            const auto &transform = view.get<TransformComponent>(entity);
            auto boundingSphere = view.get<MeshComponent>(entity).boundingSphere;
            auto material = view.get<MeshMaterialData>(entity);
            material.materialIdx = view.get<MaterialComponent>(entity).handle.index;
            data.emplace_back(
                transform.worldMatrix,
                boundingSphere,
                material,
                static_cast<uint32_t>(entity)
            );
        }
        frame.getModelBuffer().upload(data.data(), data.size() * sizeof(MeshData));
    }

    void Engine::updateSceneBuffer(FrameData &frame) const
    {
        const auto &data = mScene.getData();
        frame.getSceneBuffer().upload(&data, sizeof(SceneData));
    }

    void Engine::updateCullerBuffers(const FrameData &frame, const CommandRecorder &recorder)
    {
        std::vector<vk::DrawIndexedIndirectCommand> indirectCommands;
        auto view = mScene.getEntityRegistry().view<MeshComponent>(entt::exclude<PendingUploadComponent>);
        indirectCommands.reserve(view.size_hint());
        view.each([this, &indirectCommands](const auto &mesh)
        {
            auto meshView = mMeshRegistry.view(mesh.handle);
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
        mPickedEntity = frame.getPickerBuffer().read<uint32_t>();
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
                            auto view = mScene.getEntityRegistry().view<CameraComponent>();
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

    BodyHandle Engine::uploadPhysicsBodyDataToRegistry(const PhysicsBodyInfo &data)
    {
        return mPhysicsRegistry.createBody(data);
    }

    void Engine::addPhysicsToEntity(entt::entity entity, PhysicsCreationOptions options)
    {
        auto &reg = mScene.getEntityRegistry();

        const auto &transform = reg.get<TransformComponent>(entity).transform;

        BodyHandle handle;
        if (const auto *cache = reg.try_get<CachedPhysicsData>(entity))
        {
            std::vector<SubmeshPhysicsInfo> infos;
            infos.reserve(cache->submeshes.size());
            for (const auto &sm: cache->submeshes)
                infos.emplace_back(sm.vertices, sm.indices, sm.localTransform);

            handle = uploadPhysicsBodyDataToRegistry({
                std::move(infos),
                cache->meshType,
                transform,
                {
                    options.bodyType,
                    options.canBecomeDynamic
                }
            });
        } else if (const auto *mesh = reg.try_get<MeshComponent>(entity))
        {
            handle = uploadPhysicsBodyDataToRegistry({
                {},
                mesh->type,
                transform,
                {
                    options.bodyType,
                    options.canBecomeDynamic
                }
            });
        } else
        {
            mOnWarningLog("Cannot add physics: entity has neither cached physics data nor a mesh component");
            return;
        }

        reg.emplace<PhysicsComponent>(entity, handle, options.bodyType);
        reg.emplace<PhysicsControlComponent>(entity);
    }

    void Engine::addLightEntity(LightType type)
    {
        switch (type)
        {
            case LightType::Point:
                if (!mScene.createPointLightEntity(
                    mScene.getLightEntityName(),
                    {mGizmoRegistry.getBuiltins().pointLight, 0.5f, {1.f, 1.f, 1.f, 1.f}},
                    {}))
                    mOnWarningLog("The maximum number of point lights has been reached");
                break;
            default:
                break;
        }
    }
}
