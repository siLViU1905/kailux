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
#include "scene/SceneInstantiator.h"
#include "scene/SceneSerializer.h"
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
                                              mSimulationTextureIds(other.mSimulationTextureIds),
                                              mScene(std::move(other.mScene)),
                                              mControlledCamera(other.mControlledCamera),
                                              mInputSource(other.mInputSource),
                                              mMouseLookActive(other.mMouseLookActive),
                                              mSimulationView(std::move(other.mSimulationView)),
                                              mRetiredSimulationView(std::move(other.mRetiredSimulationView)),
                                              mSimulationResize(other.mSimulationResize),
                                              mSimulationViewActive(other.mSimulationViewActive),
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
        CreateAssetPipeline();
        CreatePhysicsSystem();
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
            mSimulationTextureIds = other.mSimulationTextureIds;
            mScene = std::move(other.mScene);
            mControlledCamera = other.mControlledCamera;
            mInputSource = other.mInputSource;
            mMouseLookActive = other.mMouseLookActive;
            mSimulationView = std::move(other.mSimulationView);
            mRetiredSimulationView = std::move(other.mRetiredSimulationView);
            mSimulationResize = other.mSimulationResize;
            mSimulationViewActive = other.mSimulationViewActive;
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

            CreateAssetPipeline();
            CreatePhysicsSystem();
        }
        return *this;
    }

    Engine::~Engine()
    {
        if (mContext.GetDevice())
        {
            WaitIdle();
            mTransferManager.Clear();
            mFrames = {};
            OneTimeCommand::destroy_command_pools();
        }
    }

    Engine Engine::create(Window &window)
    {
        Engine engine;
        engine.CreateRenderingContext(window);
        OneTimeCommand::create_command_pools(engine.mContext);
        engine.CreateMainPass();
        engine.CreateSkybox();
        engine.CreateGizmoPass();
        engine.CreateOutlinePass();
        engine.CreateTransferManager();
        engine.CreateMeshRegistry();
        engine.CreateTextureRegistry();
        engine.CreatePhysicsRegistry();
        engine.CreateGizmoRegistry();
        engine.CreateComputePicker();
        engine.CreateComputeCuller();
        engine.CreateFrameResources();
        engine.SeedDefaultTextures();
        engine.CreateImGui(window);
        engine.CreateScene(window);
        engine.CreateEditorTextureIds();
        engine.CreateAssetPipeline();
        engine.CreatePhysicsSystem();

        return engine;
    }

    void Engine::SetOnEditorRender(OnEditorRender &&callback)
    {
        mOnEditorRender = std::move(callback);
    }

    CameraData Engine::GetCameraData() const
    {
        return BuildCameraData(mScene.GetSceneCamera(), mSwapchain.GetExtent());
    }

    void Engine::SetSimulationViewExtent(vk::Extent2D extent)
    {
       mSimulationResize.Request(extent);
    }

    void Engine::SetSimulationViewActive(bool active)
    {
        mSimulationViewActive = active;
    }

    void Engine::SetControlledCamera(entt::entity camera, InputSource source)
    {
        if (mControlledCamera == camera && mInputSource == source)
            return;

        if (mMouseLookActive && mInputSource.Valid())
            mInputSource.SetCursorMode(CursorMode::Normal);

         mControlledCamera = camera;
         mInputSource      = source;
         mMouseLookActive  = false;
    }

    void Engine::ToggleMouseLook()
    {
        if (!mInputSource.Valid())
            return;

        mMouseLookActive = !mMouseLookActive;
        mInputSource.SetCursorMode(mMouseLookActive ? CursorMode::Disabled : CursorMode::Normal);

        if (mMouseLookActive)
            if (auto* camera = mScene.GetEntityRegistry().try_get<CameraComponent>(mControlledCamera))
            {
                const auto mousePos{mInputSource.GetMousePos()};
                camera->lastMousePosX = mousePos.x;
                camera->lastMousePosY = mousePos.y;
            }
    }

    void Engine::WaitIdle() const
    {
        mContext.GetDevice().waitIdle();
    }

    Queue<AssetPipeline::PendingMeshData> &Engine::GetPendingMeshDataQueue()
    {
        return mAssetPipeline.GetPendingQueue();
    }

    void Engine::UnregisterMesh(MeshHandle handle, std::string_view path)
    {
        if (auto cache = mAssetPipeline.Uncache(path))
        {
            mMeshRegistry.Destroy(cache->meshHandle);

            auto materialHandle = cache->materialHandle;
            mDeferredResourceEraser.Enqueue([this, materialHandle]()
            {
                mTextureRegistry.ReleaseMaterial(materialHandle);
            });
        }
    }

    void Engine::UnregisterMaterial(MaterialHandle handle)
    {
        mDeferredResourceEraser.Enqueue([this, handle]()
        {
            mTextureRegistry.ReleaseMaterial(handle);
        });
    }

    ImTextureID Engine::GetAssetBrowserDirectoryTextureId() const
    {
        return ImGuiBackend::get_texture_id_from_texture(mTextureRegistry.GetAssetBrowserDirectoryIconTexture());
    }

    ImTextureID Engine::GetAssetBrowserFileTextureId() const
    {
        return ImGuiBackend::get_texture_id_from_texture(mTextureRegistry.GetAssetBrowserFileIconTexture());
    }

    ImTextureID Engine::GetSceneTextureId() const
    {
        return mSceneTextureIds[mCurrentFrame];
    }

    ImTextureID Engine::GetSimulationTextureId() const
    {
        return mSimulationView.GetTextureId();
    }

    void Engine::OnEvent(const Event &event, Window &window)
    {
        if (const auto* keyReleased{std::get_if<KeyReleased>(&event)})
            if (keyReleased->key == Key::Escape && mMouseLookActive)
                ToggleMouseLook();
    }

    void Engine::CreateRenderingContext(Window &window)
    {
        mContext = Context::create(window);
        mSampleCount = mContext.GetMaxUsableSampleCount();
        mSwapchain = Swapchain::create(window, mContext, mSampleCount);
    }

    void Engine::CreateMainPass()
    {
        mMainPass = MainPass::create(
            mContext,
            mSwapchain,
            details::kFramesInFlight
        );
    }

    void Engine::CreateSkybox()
    {
        mSkyboxPass = SkyboxPass::create(
            mContext,
            mSwapchain,
            details::kFramesInFlight
        );
    }

    void Engine::CreateGizmoPass()
    {
        mGizmoPass = GizmoPass::create(mContext, mSwapchain, details::kFramesInFlight);
    }

    void Engine::CreateOutlinePass()
    {
        mOutlinePass = OutlinePass::create(
            mContext,
            mSwapchain,
            details::kFramesInFlight
        );
    }

    void Engine::CreateFrameResources()
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

    void Engine::CreateTransferManager()
    {
        mTransferManager = TransferManager::create();
    }

    void Engine::CreateMeshRegistry()
    {
        std::vector<Buffer> stagingBuffers;
        auto otc = OneTimeCommand::create(mContext);
        mMeshRegistry = MeshRegistry::create(mContext, otc.GetCommandBuffer(), stagingBuffers);
        otc.Submit(mContext);
    }

    void Engine::CreateTextureRegistry()
    {
        mTextureRegistry = TextureRegistry::create(
            mContext,
            kDirectoryIconPath,
            kFileIconPath
        );
    }

    void Engine::CreatePhysicsRegistry()
    {
        mPhysicsRegistry = PhysicsRegistry::create();
    }

    void Engine::CreateGizmoRegistry()
    {
        std::vector<Buffer> stagingBuffers;
        auto otc = OneTimeCommand::create(mContext);
        mGizmoRegistry = GizmoRegistry::create(mContext, otc.GetCommandBuffer(), stagingBuffers);
        otc.Submit(mContext);
    }

    void Engine::CreateAssetPipeline()
    {
        mAssetPipeline = AssetPipeline(mContext, mMeshRegistry, mTextureRegistry, mTransferManager, mScene, mFrames);
        mAssetPipeline.SetOnInfoLog([this](auto msg)
        {
            mOnInfoLog(msg);
        });
        mAssetPipeline.SetOnWarningLog([this](auto msg)
        {
            mOnWarningLog(msg);
        });
        mAssetPipeline.SetOnAttachPhysics([this](auto entity, auto physicsRecord)
        {
            AddPhysicsToEntity(entity, {physicsRecord.type, physicsRecord.canBecomeDynamic});
        });
    }

    void Engine::CreatePhysicsSystem()
    {
        mPhysicsSystem = PhysicsSystem(mScene, mPhysicsRegistry);
        mPhysicsSystem.SetOnWarningLog([this](auto msg)
        {
            mOnWarningLog(msg);
        });
    }

    void Engine::CreateImGui(Window &window)
    {
        mImGuiBackend = ImGuiBackend::create(window, mContext, mSwapchain, mSampleCount);
    }

    void Engine::SeedDefaultTextures()
    {
        std::vector<DescriptorSetUpdateInfo> writes;
        auto liveTextures = mTextureRegistry.GetLiveTexures();
        writes.reserve(liveTextures.size());

        for (const auto& live : liveTextures)
        {
            const Texture& tex = live.texture;
            writes.emplace_back(
                MainPass::kMeshTextureBindStart,
                live.slot,
                DescriptorSetImageInfo(
                    tex.GetSampler(),
                    tex.GetImageView(),
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    1
                )
            );
        }

        for (const auto& frame : mFrames)
            frame.GetMeshDescriptorSet().UpdateInfo(mContext, writes);
    }

    void Engine::CreateEditorTextureIds()
    {
        for (uint32_t i{}; i < details::kFramesInFlight; i++)
        {
            if (mSceneTextureIds[i])
                ImGuiBackend::remove_texture(mSceneTextureIds[i]);

            mSceneTextureIds[i] = ImGuiBackend::get_texture_id_from_texture(mFrames[i].GetSceneTexture());
        }
    }

    void Engine::CreateComputePicker()
    {
        mComputePicker = ComputePicker::create(mContext, details::kFramesInFlight);
    }

    void Engine::CreateComputeCuller()
    {
        mComputeCuller = ComputeCuller::create(mContext, details::kFramesInFlight);
    }

    void Engine::CreateScene(const Window &window)
    {
        mScene = Scene::create("MainScene", window);
    }

    void Engine::Submit(const FrameData &frame, vk::Semaphore imageAvailableSemaphore,
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

        vk::CommandBufferSubmitInfo cmdInfo{frame.GetCommandBuffer()};

        vk::SubmitInfo2 submitInfo{
            {},
            waitInfo,
            cmdInfo,
            signalInfo
        };

        mContext.GetGraphicsQueue().submit2(submitInfo, frame.GetFenceInFlight());
    }

    void Engine::Render(const Window &window)
    {
        auto &frame = mFrames[mCurrentFrame];
        frame.Reset(mContext);

        ReadOutputBuffers(frame);

        auto acquired = mSwapchain.Acquire();
        if (!acquired)
        {
            RecreateSwapchainResources(window);
            return;
        }

        if (mSimulationViewActive)
            if (const auto extent{mSimulationResize.Poll(mSimulationView.GetExtent())})
                ResizeSimulationView(*extent);

        const auto renderFinishedSemaphore = mSwapchain.GetPresentSemaphore(acquired->imageIndex); {
            CommandRecorder recorder(frame.GetCommandBuffer());
            UpdateFrameBuffers(frame, recorder);

            ExecuteCulling(frame, recorder, mScene.GetSceneCamera(), mSwapchain.GetExtent());

            TransitionForMainPass(frame, recorder);

            constexpr vk::ClearColorValue clearColor(std::array{0u, 0u, 0u, 0u});
            constexpr vk::ClearColorValue idClear(std::array{~0u, ~0u, ~0u, ~0u});

            const std::array mainAndPickerAttachments{
                ColorAttachmentInfo(
                    mSwapchain.GetColorImageView(),
                    frame.GetSceneTexture().GetImageView(),
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    clearColor,
                    vk::ResolveModeFlagBits::eAverage
                ),
                ColorAttachmentInfo(
                    frame.GetOutIdTexture().GetImageView(),
                    frame.GetResolvedOutIdTexture().GetImageView(),
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eDontCare,
                    idClear,
                    vk::ResolveModeFlagBits::eSampleZero
                )
            };

            recorder.BeginRendering(
                {
                    mainAndPickerAttachments,
                    frame.GetExtent(),
                    mSwapchain.GetDepthImageView(),
                    vk::ImageLayout::eDepthAttachmentOptimal,
                    vk::AttachmentLoadOp::eClear,
                    {}
                }
            );

            recorder.SetViewport(frame.GetExtent());
            recorder.SetScissor(frame.GetExtent());

            RecordMeshData(frame, recorder, details::kSceneCameraIndex, true);
            RecordSkybox(frame, recorder, details::kSceneCameraIndex);

            recorder.EndRendering();

            TransitionForGizmoPass(frame, recorder);

            const std::array gizmoAttachments{
                ColorAttachmentInfo(
                    mSwapchain.GetColorImageView(),
                    frame.GetSceneTexture().GetImageView(),
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eLoad,
                    vk::AttachmentStoreOp::eStore,
                    {},
                    vk::ResolveModeFlagBits::eAverage
                ),
                ColorAttachmentInfo(
                    frame.GetOutIdTexture().GetImageView(),
                    {},
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eLoad,
                    vk::AttachmentStoreOp::eStore,
                    {}
                )
            };

            recorder.BeginRendering({
                gizmoAttachments,
                frame.GetExtent(),
                mSwapchain.GetDepthImageView(),
                vk::ImageLayout::eDepthAttachmentOptimal,
                vk::AttachmentLoadOp::eLoad,
                {}
            });

            RecordGizmos(frame, recorder);
            recorder.EndRendering();

            TransitionForOutlinePass(frame, recorder, acquired->imageIndex);


            const std::array outlineAttachment{
                ColorAttachmentInfo(
                    frame.GetSceneTexture().GetImageView(),
                    {},
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eLoad,
                    vk::AttachmentStoreOp::eStore,
                    {}
                )
            };

            recorder.BeginRendering(
                {
                    outlineAttachment,
                    frame.GetExtent(),
                    {},
                    vk::ImageLayout::eUndefined,
                    {}
                });

            RecordOutline(frame, recorder);
            recorder.EndRendering();

            TransitionForPickerAndPostProcess(frame, recorder);

            if (mSimulationViewActive &&
                mSimulationView.GetExtent().width > 0 &&
                mSimulationView.GetExtent().height > 0
            )
                RenderSimulationView(frame, recorder);

            const std::array imguiOverlay{
                ColorAttachmentInfo
                (mSwapchain.GetImageView(acquired->imageIndex),
                 {},
                 vk::ImageLayout::eColorAttachmentOptimal,
                 vk::AttachmentLoadOp::eLoad,
                 vk::AttachmentStoreOp::eStore,
                 vk::ClearColorValue{std::array{0.f, 0.f, 0.f, 1.f}}
                )
            };

            recorder.BeginRendering({
                imguiOverlay,
                mSwapchain.GetExtent(),
                {},
                vk::ImageLayout::eUndefined,
                vk::AttachmentLoadOp::eClear,
                vk::RenderingFlagBits::eContentsSecondaryCommandBuffers
            });

            RecordImGuiData(frame);
            recorder.GetCommandBuffer().executeCommands(frame.GetImGuiCommandBuffer());

            recorder.EndRendering();

            TransitionForPresent(recorder, acquired->imageIndex);
        }

        Submit(mFrames[mCurrentFrame], acquired->imageAvailableSemaphore, renderFinishedSemaphore);

        mImGuiBackend.UpdatePlatform();
        if (mRetiredSimulationView.GetTextureId())
        {
            WaitIdle();
            ImGuiBackend::remove_texture(mRetiredSimulationView.GetTextureId());
            mRetiredSimulationView = {};
        }

        if (!mSwapchain.Present(mContext, acquired->imageIndex, renderFinishedSemaphore))
            RecreateSwapchainResources(window);

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

    bool Engine::IsMeshCached(std::string_view path) const
    {
        return mAssetPipeline.IsCached(path);
    }

    const Scene & Engine::GetScene() const
    {
        return mScene;
    }

    void Engine::SaveScene(const std::filesystem::path &path)
    {
        auto t{Clock::now()};
        mScene.SetSavePath(path);
        if (const auto result = SceneSerializer::save(mScene, path); !result)
        {
            mOnErrorLog(std::format("Scene '{}' not saved: {}",
                                    path.generic_string(), result.error()));
            return;
        }

        mOnInfoLog(std::format("Scene '{}' saved to '{}'",
                               mScene.GetName(), path.generic_string()));
    }

    void Engine::LoadScene(const std::filesystem::path &path, const Window &window)
    {
        auto t{Clock::now()};
        const auto document = SceneSerializer::read_file(path);
        if (!document)
        {
            mOnErrorLog(std::format("Scene '{}' not loaded: {}",
                                    path.generic_string(), document.error()));
            return;
        }

        Scene scene = Scene::create(document->meta.name, window);

        const auto fbSize{window.GetInputSource().GetFramebufferSize()};
        auto requests = SceneInstantiator::apply(scene, *document, {mGizmoRegistry, fbSize.x, fbSize.y});

        if (!requests)
        {
            mOnErrorLog(std::format("Scene '{}' not loaded: {}",
                                    path.generic_string(), requests.error()));
            return;
        }

        mScene = std::move(scene);
        mScene.SetSavePath(path);

        for (const auto &request : *requests)
        {
            AssetPipeline::PendingMeshData pending;
            pending.target    = request.target;
            pending.name      = request.name;
            pending.path      = request.record.path;
            pending.type      = request.record.type;
            pending.transform = request.transform;
            pending.material  = request.material;
            pending.physics   = request.physics;

            if (!IsMeshCached(pending.path))
                if (auto data = MeshLoader::load(pending.path))
                    pending.data = std::move(*data);

            mAssetPipeline.GetPendingQueue().Push(std::move(pending));
        }

        mOnInfoLog(std::format("Scene '{}' loaded in {}ms", mScene.GetName(),
                               Clock::get_elapsed<float, TimeType::Milliseconds>(t)));
    }

    void Engine::SetOnInfoLog(OnLog &&callback)
    {
        mOnInfoLog = std::move(callback);
        mAssetPipeline.SetOnInfoLog([this](auto msg)
        {
            mOnInfoLog(msg);
        });
    }

    void Engine::SetOnWarningLog(OnLog &&callback)
    {
        mOnWarningLog = std::move(callback);
    }

    void Engine::SetOnErrorLog(OnLog &&callback)
    {
        mOnErrorLog = std::move(callback);
    }

    void Engine::SetSceneViewportMousePos(uint32_t x, uint32_t y)
    {
        mSceneViewportMousePos = {x, y};
    }

    void Engine::SetOutlineInfo(glm::vec3 color, uint32_t entity)
    {
        mOutlineInfo = {{color, 1.f}, entity};
    }

    uint32_t Engine::GetPickedEntity() const
    {
        return mPickedEntity;
    }

    void Engine::UpdateBodyType(BodyHandle handle, PhysicsBodyType type)
    {
        mPhysicsRegistry.SetBodyType(handle, type);
    }

    void Engine::UpdateBodyScale(BodyHandle handle, const glm::vec3 &scale)
    {
        mPhysicsRegistry.UpdateBodyScale(handle, scale);
    }

    void Engine::SetSimulationState(SimulationState state)
    {
        mPhysicsSystem.SetSimulationState(state);
    }

    void Engine::ExecuteCulling(const FrameData &frame, const CommandRecorder &recorder, entt::entity camera, vk::Extent2D extent)
    {
        const auto cmd = recorder.GetCommandBuffer();

        recorder.GetCommandBuffer().fillBuffer(
            frame.GetCullerCountBuffer().GetBuffer(),
            0,
            frame.GetCullerCountBuffer().GetSize(),
            0
        );

        std::array countBufferBarrier = {frame.GetCullerCountBufferFillMemoryBarrier()};
        recorder.BufferMemoryBarriers(countBufferBarrier);

        auto totalObjects = static_cast<uint32_t>(
            mScene.GetEntityRegistry().view<MeshComponent>(entt::exclude<PendingUploadComponent>).size_hint());
        if (totalObjects == 0)
            return;

        mComputeCuller.Bind(cmd);
        frame.GetCullerDescriptorSet().Bind(mComputeCuller.GetPipeline(), cmd, vk::PipelineBindPoint::eCompute);

        const auto cameraData{BuildCameraData(camera, extent)};
        const auto planes{Camera::get_frustum_planes(cameraData.projection, cameraData.view)};
        mComputeCuller.Push<ComputePassesPushConstants::CameraFrustum>(cmd, {planes, totalObjects});

        uint32_t groupX = (totalObjects + 255) / 256;
        mComputeCuller.Execute(cmd, {groupX, 1, 1});

        recorder.BufferMemoryBarriers(frame.GetCullerBufferMemoryBarriers());
    }

    void Engine::ResizeSimulationView(vk::Extent2D extent)
    {
        WaitIdle();

        mRetiredSimulationView = std::move(mSimulationView);

        mSimulationView = SimulationView::create(
           mContext,
           mSwapchain.GetFormat(),
           mSwapchain.GetDepthFormat(),
           extent,
           mSampleCount
       );
        mSimulationView.SetTextureId(
            ImGuiBackend::get_texture_id_from_texture(mSimulationView.GetResolvedTexture()));
    }

    void Engine::TransitionForMainPass(const FrameData &frame, const CommandRecorder &recorder) const
    {
        recorder.ApplyImageBarrier({
            mSwapchain.GetColorImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal
        });

        recorder.ApplyImageBarrier({
            frame.GetSceneTexture().GetImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eShaderRead,
            vk::AccessFlagBits2::eColorAttachmentWrite
        });

        recorder.ApplyImageBarrier({
            mSwapchain.GetDepthImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests,
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::ImageAspectFlagBits::eDepth
        });

        recorder.ApplyImageBarrier({
            frame.GetOutIdTexture().GetImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eAllGraphics,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eColorAttachmentWrite
        });

        recorder.ApplyImageBarrier({
            frame.GetResolvedOutIdTexture().GetImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eAllGraphics,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eColorAttachmentWrite
        });
    }

    void Engine::TransitionForSimulationPass(const CommandRecorder &recorder) const
    {
        recorder.ApplyImageBarrier({
            mSimulationView.GetColorTexture().GetImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eColorAttachmentWrite
        });

        recorder.ApplyImageBarrier({
            mSimulationView.GetDepthTexture().GetImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests,
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::ImageAspectFlagBits::eDepth
        });

        recorder.ApplyImageBarrier({
            mSimulationView.GetResolvedTexture().GetImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eShaderRead,
            vk::AccessFlagBits2::eColorAttachmentWrite
        });
    }

    void Engine::TransitionForGizmoPass(const FrameData &frame, const CommandRecorder &recorder) const
    {
        recorder.ApplyImageBarrier({
        mSwapchain.GetColorImage(),
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite
    });

        recorder.ApplyImageBarrier({
            mSwapchain.GetDepthImage(),
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::ImageAspectFlagBits::eDepth
        });

        recorder.ApplyImageBarrier({
            frame.GetOutIdTexture().GetImage(),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite
        });
    }

    void Engine::TransitionForOutlinePass(const FrameData &frame, const CommandRecorder &recorder,
                                          uint32_t imageIndex) const
    {
        recorder.ApplyImageBarrier({
            mSwapchain.GetImage(imageIndex),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal
        });

        recorder.ApplyImageBarrier({
            frame.GetResolvedOutIdTexture().GetImage(),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eShaderRead
        });
    }

    void Engine::TransitionForPickerAndPostProcess(const FrameData &frame, const CommandRecorder &recorder) const
    {
        recorder.ApplyImageBarrier({
            frame.GetSceneTexture().GetImage(),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eShaderRead
        });

        recorder.ApplyImageBarrier({
            frame.GetResolvedOutIdTexture().GetImage(),
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eGeneral,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eShaderRead,
            vk::AccessFlagBits2::eShaderRead
        });

        RecordPicker(frame, recorder);
        std::array pickerMemBarrier{frame.GetPickerBufferMemoryBarrier()};
        recorder.BufferMemoryBarriers(pickerMemBarrier);

        recorder.ApplyImageBarrier({
            frame.GetResolvedOutIdTexture().GetImage(),
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eShaderRead,
            vk::AccessFlagBits2::eColorAttachmentWrite
        });
    }

    void Engine::TransitionForPresent(const CommandRecorder &recorder, uint32_t imageIndex) const
    {
        recorder.ApplyImageBarrier({
            mSwapchain.GetImage(imageIndex),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eBottomOfPipe,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eNone
        });
    }

    void Engine::RecordMeshData(const FrameData &frame, const CommandRecorder &recorder, uint32_t cameraIndex, bool writeIds) const
    {
        const auto cmd = recorder.GetCommandBuffer();
        mMainPass.bind(cmd, writeIds);
        mMeshRegistry.Bind(recorder.GetCommandBuffer());
        frame.GetMeshDescriptorSet().Bind(mMainPass.GetPipeline(), cmd);
        mMainPass.Push<uint32_t>(recorder.GetCommandBuffer(), cameraIndex);

        recorder.DrawIndexedIndirectCount(
            frame.GetIndirectBuffer(),
            frame.GetCullerCountBuffer(),
            details::kMaxMeshes
        );
    }

    void Engine::RecordSkybox(const FrameData &frame, const CommandRecorder &recorder, uint32_t cameraIndex) const
    {
        const auto cmd = recorder.GetCommandBuffer();
        mSkyboxPass.Bind(cmd);
        frame.GetSkyboxDescriptorSet().Bind(mSkyboxPass.GetPipeline(), cmd);
        mSkyboxPass.Push<uint32_t>(recorder.GetCommandBuffer(), cameraIndex);

        auto cubeView = mMeshRegistry.View(mMeshRegistry.GetBuiltins().cube);
        cmd.drawIndexed(
            cubeView.indexCount,
            1,
            cubeView.firstIndex,
            cubeView.vertexOffset,
            0
        );
    }

    void Engine::RecordGizmos(const FrameData &frame, const CommandRecorder &recorder) const
    {
        const auto cmd = recorder.GetCommandBuffer();
        mGizmoPass.Bind(cmd);
        mGizmoRegistry.Bind(cmd);
        frame.GetGizmoDescriptorSet().Bind(mGizmoPass.GetPipeline(), cmd);

        auto view = mScene.GetEntityRegistry().view<GizmoComponent, TransformComponent>();
        view.each([&](const auto& component, const auto& transform)
        {
            auto gizmoView = mGizmoRegistry.View(component.handle);

            const GraphicsPassesPushConstants::Gizmo pc{
                glm::vec4(transform.transform.position, component.scale),
                component.color,
                0
            };

            mGizmoPass.Push(cmd, pc);

            cmd.drawIndexed(
                gizmoView.indexCount,
                1,
                gizmoView.firstIndex,
                gizmoView.vertexOffset,
                0
                );
        });
    }

    void Engine::RecordImGuiData(const FrameData &frame)
    {
        auto format = mSwapchain.GetFormat();
        auto inheritanceInfo = vk::CommandBufferInheritanceRenderingInfo(
            {},
            {},
            1,
            &format,
            vk::Format::eUndefined,
            vk::Format::eUndefined,
            vk::SampleCountFlagBits::e1
        );

        mImGuiBackend.BeginFrame();
        mOnEditorRender(mScene);
        mImGuiBackend.EndFrame();

        CommandRecorder recorder(frame.GetImGuiCommandBuffer(), inheritanceInfo);
        mImGuiBackend.RecordDrawData(recorder.GetCommandBuffer());
    }

    void Engine::RecordPicker(const FrameData &frame, const CommandRecorder &recorder) const
    {
        const auto cmd = recorder.GetCommandBuffer();
        mComputePicker.Bind(cmd);
        frame.GetPickerDescriptorSet().Bind(mComputePicker.GetPipeline(), cmd,
                                            vk::PipelineBindPoint::eCompute);
        mComputePicker.Push(cmd, mSceneViewportMousePos);
        mComputePicker.Execute(
            cmd,
            {1, 1, 1}
        );
    }

    void Engine::RecordOutline(const FrameData &frame, const CommandRecorder &recorder) const
    {
        const auto cmd = recorder.GetCommandBuffer();
        mOutlinePass.Bind(cmd);
        frame.GetOutlineDescriptorSet().Bind(mOutlinePass.GetPipeline(), cmd);
        mOutlinePass.Push(cmd, mOutlineInfo);
        cmd.draw(3, 1, 0, 0);
    }

    void Engine::RenderSimulationView(const FrameData &frame, CommandRecorder &recorder)
    {
        const auto extent{mSimulationView.GetExtent()};

        recorder.BufferMemoryBarriers(frame.GetIndirectReadToWriteBarriers());
        ExecuteCulling(frame, recorder, mScene.GetSimulationCamera(), extent);

        TransitionForSimulationPass(recorder);

        const std::array attachments{
            ColorAttachmentInfo{
                mSimulationView.GetColorTexture().GetImageView(),
                mSimulationView.GetResolvedTexture().GetImageView(),
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::ClearColorValue{std::array{0.f, 0.f, 0.f, 1.f}},
                vk::ResolveModeFlagBits::eAverage
            }
        };

        recorder.BeginRendering({
            attachments,
            extent,
            mSimulationView.GetDepthTexture().GetImageView(),
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::AttachmentLoadOp::eClear
        });

        recorder.SetViewport(extent);
        recorder.SetScissor(extent);

        RecordMeshData(frame, recorder, details::kSimulationCameraIndex, false);
        RecordSkybox(frame, recorder, details::kSimulationCameraIndex);

        recorder.EndRendering();

        recorder.ApplyImageBarrier({
            mSimulationView.GetResolvedTexture().GetImage(),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eShaderRead
        });
    }

    CameraData Engine::BuildCameraData(entt::entity entity, vk::Extent2D extent) const
    {
        const auto& camera{mScene.GetEntityRegistry().get<CameraComponent>(entity)};
        return {
            Camera::get_projection(camera, static_cast<int>(extent.width), static_cast<int>(extent.height)),
            Camera::get_view(camera),
            {
                camera.position,
                camera.exposure
            }
        };
    }

    void Engine::Update(float deltaTime)
    {
        mAssetPipeline.Poll();
        mTransferManager.Poll(mContext);
        mDeferredResourceEraser.Tick();

        if (mPhysicsSystem.GetSimulationState() == SimulationState::Running)
            mPhysicsSystem.Update(deltaTime);

        mScene.Update();

        if (!mInputSource.Valid())
            return;

        const auto controlled{
            mControlledCamera == entt::null ?
            mScene.GetSceneCamera() :
            mControlledCamera
        };
        if (auto* camera{mScene.GetEntityRegistry().try_get<CameraComponent>(controlled)})
        {
            camera->focused = mMouseLookActive;
            Camera::update_movement(*camera, mInputSource, deltaTime);
            Camera::update_look_at(*camera, mInputSource, deltaTime);
        }
    }

    void Engine::UpdateFrameBuffers(FrameData &frame, const CommandRecorder &recorder)
    {
        UpdateCameraBuffer(frame);
        UpdateMeshDataBuffer(frame);
        UpdateMaterialBuffer(frame);
        UpdateSceneBuffer(frame);
        UpdateCullerBuffers(frame, recorder);

        recorder.BufferMemoryBarriers(frame.GetBufferMemoryBarriers());
    }

    void Engine::UpdateCameraBuffer(FrameData &frame) const
    {
        const auto simulationExtent{
            mSimulationView.GetExtent().width > 0 && mSimulationView.GetExtent().height > 0
                ? mSimulationView.GetExtent()
                : mSwapchain.GetExtent()
        };

        std::array<CameraData, details::kMaxCameras> cameras{};
        cameras[details::kSceneCameraIndex] =
                BuildCameraData(mScene.GetSceneCamera(), mSwapchain.GetExtent());
        cameras[details::kSimulationCameraIndex] =
                BuildCameraData(mScene.GetSimulationCamera(), simulationExtent);

        frame.GetCameraBuffer().Upload(
            cameras.data(),
            sizeof(CameraData) * cameras.size()
            );
    }

    void Engine::UpdateMeshDataBuffer(FrameData &frame) const
    {
        std::vector<MeshData> data;
        auto view = mScene.GetEntityRegistry().view<
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
        frame.GetModelBuffer().Upload(data.data(), data.size() * sizeof(MeshData));
    }

    void Engine::UpdateMaterialBuffer(FrameData &frame) const
    {
        const auto materials = mTextureRegistry.ViewMaterials();
        frame.GetMaterialBuffer().Upload(materials);
    }

    void Engine::UpdateSceneBuffer(FrameData &frame) const
    {
        const auto &data = mScene.GetData();
        frame.GetSceneBuffer().Upload(&data, sizeof(SceneData));
    }

    void Engine::UpdateCullerBuffers(const FrameData &frame, const CommandRecorder &recorder)
    {
        std::vector<vk::DrawIndexedIndirectCommand> indirectCommands;
        auto view = mScene.GetEntityRegistry().view<MeshComponent>(entt::exclude<PendingUploadComponent>);
        indirectCommands.reserve(view.size_hint());
        view.each([this, &indirectCommands](const auto &mesh)
        {
            auto meshView = mMeshRegistry.View(mesh.handle);
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

        frame.GetCullerInputCommandsBuffer().Upload(indirectCommands.data(),
                                                    indirectCommands.size() * sizeof(vk::DrawIndexedIndirectCommand));
    }

    void Engine::ReadOutputBuffers(const FrameData &frame)
    {
        mPickedEntity = frame.GetPickerBuffer().Read<uint32_t>();
    }

    void Engine::RecreateSwapchainResources(const Window &window)
    {
        mSwapchain.Recreate(window, mContext, mSampleCount);
        for (auto &f : mFrames)
            f.RecreateTextures(mContext, mSwapchain);
        CreateEditorTextureIds();
    }

    BodyHandle Engine::UploadPhysicsBodyDataToRegistry(const PhysicsBodyInfo &data)
    {
        auto t = Clock::now();
        auto handle = mPhysicsRegistry.CreateBody(data);
        mOnInfoLog(std::format("Physics body attached in {:.3f}ms", Clock::get_elapsed<float, TimeType::Milliseconds>(t)));
        return handle;
    }

    void Engine::AddPhysicsToEntity(entt::entity entity, PhysicsCreationOptions options)
    {
        auto &reg = mScene.GetEntityRegistry();

        const auto &transform = reg.get<TransformComponent>(entity).transform;

        BodyHandle handle;
        if (const auto *cache = reg.try_get<CachedPhysicsData>(entity))
        {
            std::vector<SubmeshPhysicsInfo> infos;
            infos.reserve(cache->submeshes.size());
            for (const auto &sm: cache->submeshes)
                infos.emplace_back(sm.vertices, sm.indices, sm.localTransform);

            handle = UploadPhysicsBodyDataToRegistry({
                std::move(infos),
                cache->meshType,
                transform,
                {
                    options.bodyType,
                    options.canBecomeDynamic
                }
            });
        } else if (const auto *source = reg.try_get<MeshSourceComponent>(entity))
        {
            handle = UploadPhysicsBodyDataToRegistry({
                {},
                source->type,
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

        mScene.AttachPhysics(entity, {handle, options.bodyType, options.canBecomeDynamic});
    }

    void Engine::AddLightEntity(LightType type)
    {
        switch (type)
        {
            case LightType::Point:
                if (!mScene.CreatePointLightEntity(
                    mScene.GetLightEntityName(),
                    {mGizmoRegistry.GetBuiltins().pointLight, 0.5f, {1.f, 1.f, 1.f, 1.f}},
                    {}))
                    mOnWarningLog("The maximum number of point lights has been reached");
                break;
            default:
                break;
        }
    }

    DeviceInfo Engine::GetDeviceInfo() const
    {
        return mContext.GetDeviceInfo();
    }
}
