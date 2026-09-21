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
#include "components/entt/SceneSettings.h"
#include "components/entt/TagComponent.h"
#include "components/entt/WorldTransform.h"
#include "components/gpu/CameraData.h"
#include "components/gpu/MeshData.h"
#include "scene/SceneInstantiator.h"
#include "scene/SceneSerializer.h"
#include "shadow/ShadowCascades.h"
#include "texture/TextureAllocator.h"

namespace kailux
{
    Engine::Engine() : mAssetPipeline(mContext, mMeshRegistry, mTextureRegistry, mTransferManager, mScene,mFrames),
                       mPhysicsSystem(mScene, mPhysicsRegistry),
                       mCurrentFrame(0),
                       mPickedEntity(~0u)
    {
    }

    Engine::Engine(Engine &&other) noexcept : mContext(std::move(other.mContext)),
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
                                              mScene(std::move(other.mScene)),
                                              mControlledCamera(other.mControlledCamera),
                                              mInputSource(other.mInputSource),
                                              mMouseLookActive(other.mMouseLookActive),
                                              mSceneViews(std::move(other.mSceneViews)),
                                              mSimulationView(std::move(other.mSimulationView)),
                                              mRetiredViews(std::move(other.mRetiredViews)),
                                              mSceneResize(other.mSceneResize),
                                              mSimulationResize(other.mSimulationResize),
                                              mSimulationViewActive(other.mSimulationViewActive),
                                              mSceneViewExtent(other.mSceneViewExtent),
                                              mRenderScale(other.mRenderScale),
                                              mSimulationSamples(other.mSimulationSamples),
                                              mMainPass(std::move(other.mMainPass)),
                                              mSkyboxPass(std::move(other.mSkyboxPass)),
                                              mGizmoPass(std::move(other.mGizmoPass)),
                                              mOutlinePass(std::move(other.mOutlinePass)),
                                              mComputePicker(std::move(other.mComputePicker)),
                                              mPickedEntity(other.mPickedEntity),
                                              mComputeCuller(std::move(other.mComputeCuller)),
                                              mShadowPass(std::move(other.mShadowPass)),
                                              mDirectionalShadowMap(std::move(other.mDirectionalShadowMap)),
                                              mPointShadowMap(std::move(other.mPointShadowMap)),
                                              mDirectionalShadowSets(other.mDirectionalShadowSets),
                                              mPointShadowSets(other.mPointShadowSets),
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
            mScene = std::move(other.mScene);
            mControlledCamera = other.mControlledCamera;
            mInputSource = other.mInputSource;
            mMouseLookActive = other.mMouseLookActive;
            mSceneViews = std::move(other.mSceneViews);
            mSimulationView = std::move(other.mSimulationView);
            mRetiredViews = std::move(other.mRetiredViews);
            mSceneResize = other.mSceneResize;
            mSimulationResize = other.mSimulationResize;
            mSimulationViewActive = other.mSimulationViewActive;
            mSceneViewExtent = other.mSceneViewExtent;
            mRenderScale = other.mRenderScale;
            mSimulationSamples = other.mSimulationSamples;
            mMainPass = std::move(other.mMainPass);
            mSkyboxPass = std::move(other.mSkyboxPass);
            mGizmoPass = std::move(other.mGizmoPass);
            mOutlinePass = std::move(other.mOutlinePass);
            mComputePicker = std::move(other.mComputePicker);
            mPickedEntity = other.mPickedEntity;
            mComputeCuller = std::move(other.mComputeCuller);
            mShadowPass = std::move(other.mShadowPass);
            mDirectionalShadowMap = std::move(other.mDirectionalShadowMap);
            mPointShadowMap = std::move(other.mPointShadowMap);
            mDirectionalShadowSets = other.mDirectionalShadowSets;
            mPointShadowSets = other.mPointShadowSets;
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
        engine.CreateShadowPass();
        engine.CreateTransferManager();
        engine.CreateMeshRegistry();
        engine.CreateTextureRegistry();
        engine.CreatePhysicsRegistry();
        engine.CreateGizmoRegistry();
        engine.CreateComputePicker();
        engine.CreateComputeCuller();
        engine.CreateDirectionalShadowMap();
        engine.CreatePointShadowMap();

        engine.CreateSceneViews();
        engine.CreateFrameResources();

        engine.SeedDefaultTextures();
        engine.CreateImGui(window);
        engine.AcquireViewTextureIds();
        engine.CreateScene(window);
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
        const auto &sceneView{mSceneViews[mCurrentFrame]};
        return BuildCameraData(mScene.GetSceneCamera(), sceneView.GetExtent());
    }

    void Engine::SetSceneViewExtent(glm::ivec2 extent)
    {
        mSceneViewExtent = extent;
        mSceneResize.Request(apply_scale(extent, mRenderScale));
    }

    void Engine::SetSimulationViewExtent(glm::ivec2 extent)
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
        return mSceneViews[mCurrentFrame].GetTextureId();
    }

    ImTextureID Engine::GetSimulationTextureId() const
    {
        return mSimulationView.GetTextureId();
    }

    void Engine::OnEvent(const Event &event, Window &window)
    {
        if (const auto* keyReleased{std::get_if<KeyReleased>(&event)})
        {
            const auto key{keyReleased->key};
            if (key == Key::Escape && mMouseLookActive)
                ToggleMouseLook();
            else if (key == Key::Tab)
                mScene.SetPrimaryCamera(mScene.GetNextCamera(mScene.GetPrimaryCamera()));
        }
    }

    void Engine::CreateRenderingContext(Window &window)
    {
        mContext = Context::create(window);
        mSimulationSamples = mContext.GetMaxUsableSampleCount();
        mSwapchain = Swapchain::create(window, mContext, kSceneSamples);
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

    void Engine::CreateShadowPass()
    {
        mShadowPass = ShadowPass::create(
            mContext,
            mSwapchain,
            details::kFramesInFlight
            );
    }

    void Engine::CreateFrameResources()
    {
        for (uint32_t i{}; i < details::kFramesInFlight; ++i)
            mFrames[i] = FrameData::create(
                mContext,
                mSwapchain,
                mSceneViews[i],
                mMainPass,
                mSkyboxPass,
                mGizmoPass,
                mComputePicker,
                mOutlinePass,
                mComputeCuller,
                mShadowPass,
                mDirectionalShadowMap,
                mPointShadowMap,
                mTextureRegistry
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
        mPhysicsSystem.SetOnInfoLog([this](auto msg)
        {
            mOnInfoLog(msg);
        });
        mPhysicsSystem.SetOnWarningLog([this](auto msg)
        {
            mOnWarningLog(msg);
        });
    }

    void Engine::CreateImGui(Window &window)
    {
        mImGuiBackend = ImGuiBackend::create(window, mContext, mSwapchain, kSceneSamples);
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
        mScene = Scene::create("MainScene");
    }

    void Engine::CreateDirectionalShadowMap()
    {
        mDirectionalShadowMap = ShadowMap::create(
            mContext,
            details::kShadowMapResolution,
            details::kShadowCascadeCount * details::kMaxCameraViews,
            mSwapchain.GetDepthFormat(),
            false
        );
    }

    void Engine::CreatePointShadowMap()
    {
        mPointShadowMap = ShadowMap::create(
            mContext,
            details::kPointShadowResolution,
            details::kMaxPointShadows * details::kPointShadowFaceCount * details::kMaxCameraViews,
            mSwapchain.GetDepthFormat(),
            true
        );
    }

    void Engine::CreateSceneViews()
    {
        const glm::ivec2 initialExtent{
            static_cast<int>(mSwapchain.GetExtent().width),
            static_cast<int>(mSwapchain.GetExtent().height)
        };

        for (auto &view : mSceneViews)
            view = RenderTarget::create(mContext, {
                mSwapchain.GetFormat(),
                mSwapchain.GetDepthFormat(),
                initialExtent,
                kSceneSamples,
                true
            });

        mSceneViewExtent = initialExtent;

        mSimulationView = RenderTarget::create(mContext, {
            mSwapchain.GetFormat(),
            mSwapchain.GetDepthFormat(),
            initialExtent,
            mSimulationSamples,
            false
        });
    }

    void Engine::AcquireViewTextureIds()
    {
        for (auto &view : mSceneViews)
            view.SetTextureId(
                ImGuiBackend::get_texture_id_from_texture(view.GetPresentedTexture()));

        mSimulationView.SetTextureId(
            ImGuiBackend::get_texture_id_from_texture(mSimulationView.GetPresentedTexture()));
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
        auto &frame{mFrames[mCurrentFrame]};
        const auto &sceneView{mSceneViews[mCurrentFrame]};

        frame.Reset(mContext);
        ReadOutputBuffers(frame);

        const auto acquired{mSwapchain.Acquire()};
        if (!acquired)
        {
            RecreateSwapchainResources(window);
            return;
        }

        if (const auto extent{mSceneResize.Poll(sceneView.GetExtent())})
            ResizeSceneView(*extent);

        if (mSimulationViewActive)
            if (const auto extent{mSimulationResize.Poll(mSimulationView.GetExtent())})
                ResizeSimulationView(*extent);

        const auto sceneExtent{sceneView.GetVkExtent()};

        const auto renderFinishedSemaphore = mSwapchain.GetPresentSemaphore(acquired->imageIndex); {
            CommandRecorder recorder(frame.GetCommandBuffer());

            mDirectionalShadowSets[details::kSceneViewCameraIndex].Update(
               mScene,
               mScene.GetSceneCamera(),
               mSceneViews[mCurrentFrame].GetExtent()
               );
            mDirectionalShadowSets[details::kSimulationViewCameraIndex].Update(
                mScene,
                mSimulationViewActive ? mScene.GetPrimaryCamera() : entt::null,
                mSimulationView.GetExtent()
            );
            mPointShadowSets[details::kSceneViewCameraIndex].Update(mScene, mScene.GetSceneCamera());
            mPointShadowSets[details::kSimulationViewCameraIndex].Update(
                mScene,
                mSimulationViewActive ? mScene.GetPrimaryCamera() : entt::null
            );

            UpdateFrameBuffers(frame, sceneView, recorder);
            ExecuteCulling(frame, recorder, mScene.GetSceneCamera(), sceneExtent, CullingPreset::SceneView);

            TransitionForShadowPass(frame, recorder);
            for (uint32_t view{}; view < details::kMaxCameraViews; ++view)
                RecordDirectionalShadows(frame, recorder, view);
            TransitionShadowMapForSampling(frame, recorder);

            TransitionForPointShadowPass(frame, recorder);
            for (uint32_t view{}; view < details::kMaxCameraViews; ++view)
                RecordPointShadows(frame, recorder, view);
            TransitionPointShadowMapForSampling(frame, recorder);

            TransitionForMainPass(sceneView, recorder);

            constexpr vk::ClearColorValue clearColor(std::array{0u, 0u, 0u, 0u});
            constexpr vk::ClearColorValue idClear(std::array{~0u, ~0u, ~0u, ~0u});

            const std::array mainAndPickerAttachments{
                ColorAttachmentInfo(
                    sceneView.GetColorTexture().GetImageView(),
                    sceneView.GetResolveView(),
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    clearColor,
                    sceneView.IsMultisampled()
                        ? vk::ResolveModeFlagBits::eAverage
                        : vk::ResolveModeFlagBits::eNone
                ),
                ColorAttachmentInfo(
                    sceneView.GetIdTexture().GetImageView(),
                    sceneView.IsMultisampled()
                        ? sceneView.GetResolvedIdTexture().GetImageView()
                        : vk::ImageView{},
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eClear,
                    sceneView.IsMultisampled()
                        ? vk::AttachmentStoreOp::eDontCare
                        : vk::AttachmentStoreOp::eStore,
                    idClear,
                    sceneView.IsMultisampled()
                        ? vk::ResolveModeFlagBits::eSampleZero
                        : vk::ResolveModeFlagBits::eNone
                )
            };

            recorder.BeginRendering({
                mainAndPickerAttachments,
                sceneExtent,
                sceneView.GetDepthTexture().GetImageView(),
                vk::ImageLayout::eDepthAttachmentOptimal,
                vk::AttachmentLoadOp::eClear,
                {}
            });

            recorder.SetViewport(sceneExtent);
            recorder.SetScissor(sceneExtent);

            RecordMeshData(frame, recorder, details::kSceneViewCameraIndex, true);
            RecordSkybox(frame, recorder, details::kSceneViewCameraIndex, sceneView.IsMultisampled());

            recorder.EndRendering();

            TransitionForGizmoPass(sceneView, recorder);

            const std::array gizmoAttachments{
                ColorAttachmentInfo(
                    sceneView.GetColorTexture().GetImageView(),
                    sceneView.GetResolveView(),
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eLoad,
                    vk::AttachmentStoreOp::eStore,
                    {},
                    sceneView.IsMultisampled()
                        ? vk::ResolveModeFlagBits::eAverage
                        : vk::ResolveModeFlagBits::eNone
                ),
                ColorAttachmentInfo(
                    sceneView.GetIdTexture().GetImageView(),
                    {},
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eLoad,
                    vk::AttachmentStoreOp::eStore,
                    {}
                )
            };

            recorder.BeginRendering({
                gizmoAttachments,
                sceneExtent,
                sceneView.GetDepthTexture().GetImageView(),
                vk::ImageLayout::eDepthAttachmentOptimal,
                vk::AttachmentLoadOp::eLoad,
                {}
            });

            RecordGizmos(frame, recorder);
            recorder.EndRendering();

            TransitionForOutlinePass(sceneView, recorder, acquired->imageIndex);

            const std::array outlineAttachment{
                ColorAttachmentInfo(
                    sceneView.GetPresentedTexture().GetImageView(),
                    {},
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eLoad,
                    vk::AttachmentStoreOp::eStore,
                    {}
                )
            };

            recorder.BeginRendering({
                outlineAttachment,
                sceneExtent,
                {},
                vk::ImageLayout::eUndefined,
                {}
            });

            RecordOutline(frame, recorder);
            recorder.EndRendering();

            TransitionForPickerAndPostProcess(frame, sceneView, recorder);

            RecordPicker(frame, recorder);

            const std::array pickerMemBarrier{frame.GetPickerBufferMemoryBarrier()};
            recorder.BufferMemoryBarriers(pickerMemBarrier);

            if (mSimulationViewActive &&
                mSimulationView.GetExtent().x > 0 &&
                mSimulationView.GetExtent().y > 0
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

        if (!mSwapchain.Present(mContext, acquired->imageIndex, renderFinishedSemaphore))
            RecreateSwapchainResources(window);

        RetireViews();

        mCurrentFrame = (mCurrentFrame + 1) % details::kFramesInFlight;
    }

    bool Engine::is_mesh_type_supported(std::string_view path)
    {
        using namespace std::string_view_literals;
        static constexpr std::array supported =
        {
            "fbx"sv,
            "gltf"sv,
            "obj"sv
        };

        auto extension = path.substr(path.find_last_of('.') + 1);

        return std::ranges::contains(supported, extension);
    }

    bool Engine::is_image_type_supported(std::string_view path)
    {
        using namespace std::string_view_literals;
        static constexpr std::array supported =
        {
            "jpeg"sv,
            "png"sv
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

        Scene scene = Scene::create(document->meta.name);

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
        mSceneViewportMousePos = {
            static_cast<uint32_t>(x * mRenderScale),
            static_cast<uint32_t>(y * mRenderScale)
        };
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

    bool Engine::RequestSimulationState(SimulationState state)
    {
        if (state == SimulationState::Paused)
        {
            mPhysicsSystem.SetSimulationState(state);
            return true;
        }

        if (mScene.GetPrimaryCamera() == entt::null)
        {
            mOnWarningLog("Cannot start simulation: the scene has no camera");
            return false;
        }
        mScene.UpdateCameras();
        mPhysicsSystem.SetSimulationState(state);
        return true;
    }

    void Engine::ExecuteCulling(const FrameData &frame, const CommandRecorder &recorder, entt::entity camera, vk::Extent2D extent, CullingPreset preset)
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

        const auto totalObjects{mScene.GetEntityCount<MeshComponent>(entt::exclude<PendingUploadComponent>)};
        if (totalObjects == 0)
            return;

        mComputeCuller.Bind(cmd);
        frame.GetCullerDescriptorSet().Bind(mComputeCuller.GetPipeline(), cmd);

        const auto cameraData{BuildCameraData(camera, {extent.width, extent.height})};

        const auto planes{Camera::get_frustum_planes(cameraData.projection, cameraData.view)};

        const float projFactor{
            0.5f * static_cast<float>(extent.height) * std::abs(cameraData.projection[1][1])
        };
        const glm::vec4 cameraPosition{glm::vec3(cameraData.positionAndExposure), projFactor};

        const auto& settings{mScene.GetEntityRegistry().get<SceneSettings>(mScene.GetSettingsEntity())};

        mComputeCuller.Push<ComputePassesPushConstants::CullParams>(
            cmd,
            {
                planes,
                cameraPosition,
                {
                    static_cast<float>(totalObjects),
                    preset == CullingPreset::SceneView
                        ? settings.sceneMaxLod
                        : settings.simulationMaxLod,
                    preset == CullingPreset::SceneView
                        ? settings.sceneLodErrorThreshold
                        : settings.simulationLodErrorThreshold,
                    0.f
                }
            }
        );

        uint32_t groupX = (totalObjects + 255) / 256;
        mComputeCuller.Execute(cmd, {groupX, 1, 1});

        recorder.BufferMemoryBarriers(frame.GetCullerBufferMemoryBarriers());
    }

    void Engine::ResizeSceneView(glm::ivec2 extent)
    {
        if (extent.x <= 0 || extent.y <= 0)
            return;

        WaitIdle();

        for (auto &view : mSceneViews)
        {
            if (view.GetTextureId())
                mRetiredViews.push_back(std::move(view));

            view = RenderTarget::create(mContext, {
                mSwapchain.GetFormat(),
                mSwapchain.GetDepthFormat(),
                extent,
                kSceneSamples,
                true
            });
            view.SetTextureId(
                ImGuiBackend::get_texture_id_from_texture(view.GetPresentedTexture()));
        }

        for (uint32_t i{}; i < details::kFramesInFlight; ++i)
            mFrames[i].RebindSceneViewTextures(mContext, mSceneViews[i]);
    }

    void Engine::ResizeSimulationView(glm::ivec2 extent)
    {
        if (extent.x <= 0 || extent.y <= 0)
            return;

        WaitIdle();

        if (mSimulationView.GetTextureId())
            mRetiredViews.push_back(std::move(mSimulationView));

        mSimulationView = RenderTarget::create(mContext, {
            mSwapchain.GetFormat(),
            mSwapchain.GetDepthFormat(),
            extent,
            mSimulationSamples,
            false
        });

        mSimulationView.SetTextureId(
            ImGuiBackend::get_texture_id_from_texture(mSimulationView.GetPresentedTexture()));
    }

    void Engine::RetireViews()
    {
        if (mRetiredViews.empty())
            return;

        WaitIdle();
        for (auto &view : mRetiredViews)
            if (view.GetTextureId())
                ImGuiBackend::remove_texture(view.GetTextureId());
        mRetiredViews.clear();
    }

    glm::ivec2 Engine::apply_scale(glm::ivec2 extent, float scale)
    {
        return {
            std::max(1, static_cast<int>(std::lround(extent.x * scale))),
            std::max(1, static_cast<int>(std::lround(extent.y * scale)))
        };
    }

    void Engine::TransitionForShadowPass(const FrameData &frame, const CommandRecorder &recorder) const
    {
        recorder.ApplyImageBarrier({
                mDirectionalShadowMap.GetImage(),
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eDepthAttachmentOptimal,
                vk::PipelineStageFlagBits2::eFragmentShader,
                vk::PipelineStageFlagBits2::eEarlyFragmentTests,
                vk::AccessFlagBits2::eShaderRead,
                vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                vk::ImageAspectFlagBits::eDepth,
                details::kShadowCascadeCount * details::kMaxCameraViews
            }
        );
    }

    void Engine::TransitionShadowMapForSampling(const FrameData &frame, const CommandRecorder &recorder) const
    {
        recorder.ApplyImageBarrier({
            mDirectionalShadowMap.GetImage(),
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits2::eShaderRead,
            vk::ImageAspectFlagBits::eDepth,
            details::kShadowCascadeCount * details::kMaxCameraViews
        });
    }

    void Engine::TransitionForPointShadowPass(const FrameData &frame, const CommandRecorder &recorder) const
    {
        recorder.ApplyImageBarrier({
            mPointShadowMap.GetImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests,
            vk::AccessFlagBits2::eShaderRead,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::ImageAspectFlagBits::eDepth,
            details::kMaxPointShadows * details::kPointShadowFaceCount * details::kMaxCameraViews
        });
    }

    void Engine::TransitionPointShadowMapForSampling(const FrameData &frame, const CommandRecorder &recorder) const
    {
        recorder.ApplyImageBarrier({
           mPointShadowMap.GetImage(),
           vk::ImageLayout::eDepthAttachmentOptimal,
           vk::ImageLayout::eShaderReadOnlyOptimal,
           vk::PipelineStageFlagBits2::eLateFragmentTests,
           vk::PipelineStageFlagBits2::eFragmentShader,
           vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
           vk::AccessFlagBits2::eShaderRead,
           vk::ImageAspectFlagBits::eDepth,
           details::kMaxPointShadows * details::kPointShadowFaceCount * details::kMaxCameraViews
       });
    }

    void Engine::TransitionForMainPass(const RenderTarget &sceneView, const CommandRecorder &recorder) const
    {
        recorder.ApplyImageBarrier({
            sceneView.GetColorTexture().GetImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eShaderRead,
            vk::AccessFlagBits2::eColorAttachmentWrite
        });

        recorder.ApplyImageBarrier({
            sceneView.GetDepthTexture().GetImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests,
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::ImageAspectFlagBits::eDepth
        });

        recorder.ApplyImageBarrier({
            sceneView.GetIdTexture().GetImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eAllGraphics,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eColorAttachmentWrite
        });


        if (sceneView.IsMultisampled())
            recorder.ApplyImageBarrier({
                sceneView.GetResolvedIdTexture().GetImage(),
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

        if (mSimulationView.IsMultisampled())
            recorder.ApplyImageBarrier({
                mSimulationView.GetPresentedTexture().GetImage(),
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::PipelineStageFlagBits2::eFragmentShader,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eShaderRead,
                vk::AccessFlagBits2::eColorAttachmentWrite
            });
    }

    void Engine::TransitionForGizmoPass(const RenderTarget &sceneView, const CommandRecorder &recorder) const
    {
        recorder.ApplyImageBarrier({
            sceneView.GetColorTexture().GetImage(),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite
        });

        if (sceneView.IsMultisampled())
            recorder.ApplyImageBarrier({
                sceneView.GetPresentedTexture().GetImage(),
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eColorAttachmentWrite,
                vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite
            });

        recorder.ApplyImageBarrier({
            sceneView.GetDepthTexture().GetImage(),
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::ImageAspectFlagBits::eDepth
        });

        recorder.ApplyImageBarrier({
            sceneView.GetIdTexture().GetImage(),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite
        });
    }

    void Engine::TransitionForOutlinePass(const RenderTarget &sceneView, const CommandRecorder &recorder,
                                          uint32_t imageIndex) const
    {
        recorder.ApplyImageBarrier({
            mSwapchain.GetImage(imageIndex),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal
        });

        recorder.ApplyImageBarrier({
            sceneView.GetReadableIdTexture().GetImage(),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eShaderRead
        });

        recorder.ApplyImageBarrier({
            sceneView.GetPresentedTexture().GetImage(),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite
        });
    }

    void Engine::TransitionForPickerAndPostProcess(const FrameData& frame, const RenderTarget &sceneView, const CommandRecorder &recorder) const
    {
        recorder.ApplyImageBarrier({
            sceneView.GetPresentedTexture().GetImage(),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eShaderRead
        });

        recorder.ApplyImageBarrier({
            sceneView.GetReadableIdTexture().GetImage(),
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eGeneral,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eShaderRead,
            vk::AccessFlagBits2::eShaderRead
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
        mMainPass.Bind(cmd, writeIds);
        mMeshRegistry.Bind(recorder.GetCommandBuffer());
        frame.GetMeshDescriptorSet().Bind(mMainPass.GetPipeline(), cmd);
        mMainPass.Push<uint32_t>(recorder.GetCommandBuffer(), cameraIndex);

        recorder.DrawIndexedIndirectCount(
            frame.GetIndirectBuffer(),
            frame.GetCullerCountBuffer(),
            details::kMaxMeshes
        );
    }

    void Engine::RecordSkybox(const FrameData &frame, const CommandRecorder &recorder, uint32_t cameraIndex, bool multisampled) const
    {
        const auto cmd = recorder.GetCommandBuffer();
        mSkyboxPass.Bind(cmd, multisampled);
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

    void Engine::RecordDirectionalShadows(const FrameData &frame, CommandRecorder &recorder, uint32_t viewIndex) const
    {
        const auto cmd{recorder.GetCommandBuffer()};
        const auto extent{mDirectionalShadowMap.GetExtent()};

        const auto objectCount{mScene.GetEntityCount<MeshComponent>(entt::exclude<PendingUploadComponent>)};
        const auto& shadowSet{mDirectionalShadowSets[viewIndex]};
        const bool castShadows{objectCount > 0 && shadowSet.Enabled()};

        for (uint32_t cascade{}; cascade < details::kShadowCascadeCount; ++cascade)
        {
            recorder.BeginRendering({
                {},
                extent,
                mDirectionalShadowMap.GetLayerView(viewIndex * details::kShadowCascadeCount + cascade),
                vk::ImageLayout::eDepthAttachmentOptimal,
                vk::AttachmentLoadOp::eClear
            });

            if (castShadows)
            {
                recorder.SetViewport(extent);
                recorder.SetScissor(extent);

                mShadowPass.Bind(cmd);
                mMeshRegistry.Bind(cmd);
                frame.GetShadowDescriptorSet().Bind(mShadowPass.GetPipeline(), cmd);
                mShadowPass.Push(cmd, GraphicsPassesPushConstants::ShadowCascade{
                    shadowSet.GetCascade(cascade)
                });

                cmd.drawIndexedIndirect(
                    frame.GetCullerInputCommandsBuffer().GetBuffer(),
                    0,
                    objectCount,
                    details::kMaxGeometryLods * sizeof(vk::DrawIndexedIndirectCommand)
                );
            }

            recorder.EndRendering();
        }
    }

    void Engine::RecordPointShadows(const FrameData &frame, CommandRecorder &recorder, uint32_t viewIndex) const
    {
        const auto cmd{recorder.GetCommandBuffer()};
        const auto extent{mPointShadowMap.GetExtent()};
        const auto &shadowSet{mPointShadowSets[viewIndex]};

        const auto objectCount{mScene.GetEntityCount<MeshComponent>(entt::exclude<PendingUploadComponent>)};

        for (uint32_t slot{}; slot < details::kMaxPointShadows; ++slot)
        {
            if (objectCount == 0 || !shadowSet.NeedsRedraw(slot))
                continue;

            for (uint32_t face{}; face < details::kPointShadowFaceCount; ++face)
            {
                const uint32_t cube{viewIndex * details::kMaxPointShadows + slot};
                const uint32_t layer{cube * details::kPointShadowFaceCount + face};

                recorder.BeginRendering({
                    {},
                    extent,
                    mPointShadowMap.GetLayerView(layer),
                    vk::ImageLayout::eDepthAttachmentOptimal,
                    vk::AttachmentLoadOp::eClear
                });

                recorder.SetViewportNoFlip(extent);
                recorder.SetScissor(extent);

                mShadowPass.Bind(cmd);
                mMeshRegistry.Bind(cmd);
                frame.GetShadowDescriptorSet().Bind(mShadowPass.GetPipeline(), cmd);
                mShadowPass.Push(cmd, GraphicsPassesPushConstants::ShadowCascade{
                                     shadowSet.GetFace(slot, face)
                                 });

                cmd.drawIndexedIndirect(
                    frame.GetCullerInputCommandsBuffer().GetBuffer(),
                    0,
                    objectCount,
                    details::kMaxGeometryLods * sizeof(vk::DrawIndexedIndirectCommand)
                );

                recorder.EndRendering();
            }
        }
    }

    void Engine::RecordGizmos(const FrameData &frame, const CommandRecorder &recorder) const
    {
        const auto cmd = recorder.GetCommandBuffer();
        mGizmoPass.Bind(cmd);
        mGizmoRegistry.Bind(cmd);
        frame.GetGizmoDescriptorSet().Bind(mGizmoPass.GetPipeline(), cmd);

        const auto view{mScene.GetEntityRegistry().view<GizmoComponent, WorldTransform>()};
        view.each([&](const auto& component, const auto& world)
        {
            auto gizmoView = mGizmoRegistry.View(component.handle);

            const GraphicsPassesPushConstants::Gizmo pc{
                glm::vec4(world.GetPosition(), component.scale),
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
        frame.GetPickerDescriptorSet().Bind(mComputePicker.GetPipeline(), cmd);
        mComputePicker.Push(cmd, mSceneViewportMousePos);
        mComputePicker.Execute(
            cmd,
            {1, 1, 1}
        );
    }

    void Engine::RecordOutline(const FrameData &frame, const CommandRecorder &recorder)
    {
        const auto cmd = recorder.GetCommandBuffer();
        mOutlinePass.Bind(cmd);
        frame.GetOutlineDescriptorSet().Bind(mOutlinePass.GetPipeline(), cmd);
        mOutlineInfo.color = {mScene.GetEntityRegistry().get<SceneSettings>(mScene.GetSettingsEntity()).outlineColor, 0.f};
        mOutlinePass.Push(cmd, mOutlineInfo);
        cmd.draw(3, 1, 0, 0);
    }

    void Engine::RenderSimulationView(const FrameData &frame, CommandRecorder &recorder)
    {
        const auto primaryCamera{mScene.GetPrimaryCamera()};
        if (primaryCamera == entt::null)
            return;

        const vk::Extent2D extent{
            static_cast<uint32_t>(mSimulationView.GetExtent().x),
            static_cast<uint32_t>(mSimulationView.GetExtent().y)
        };

        recorder.BufferMemoryBarriers(frame.GetIndirectReadToWriteBarriers());
        ExecuteCulling(frame, recorder, primaryCamera, extent, CullingPreset::SimulationView);

        TransitionForSimulationPass(recorder);

        const std::array attachments{
            ColorAttachmentInfo{
                mSimulationView.GetColorTexture().GetImageView(),
                mSimulationView.GetResolveView(),
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::ClearColorValue{std::array{0.f, 0.f, 0.f, 1.f}},
                mSimulationView.IsMultisampled()
                    ? vk::ResolveModeFlagBits::eAverage
                    : vk::ResolveModeFlagBits::eNone
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

        RecordMeshData(frame, recorder, details::kSimulationViewCameraIndex, false);
        RecordSkybox(frame, recorder, details::kSimulationViewCameraIndex, mSimulationView.IsMultisampled());

        recorder.EndRendering();

        recorder.ApplyImageBarrier({
            mSimulationView.GetPresentedTexture().GetImage(),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eShaderRead
        });
    }

    CameraData Engine::BuildCameraData(entt::entity entity, glm::ivec2 extent) const
    {
        const auto& camera{mScene.GetEntityRegistry().get<CameraComponent>(entity)};
        return {
            Camera::get_projection(camera, extent.x, extent.y),
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
        {
            mScene.UpdateCameras();
            return;
        }

        const auto controlled{
            mControlledCamera == entt::null ?
            mScene.GetSceneCamera() :
            mControlledCamera
        };

        auto& registry{mScene.GetEntityRegistry()};
        if (registry.valid(controlled))
            if (auto* camera{registry.try_get<CameraComponent>(controlled)})
            {
                camera->focused = mMouseLookActive;
                Camera::update_movement(*camera, mInputSource, deltaTime);
                Camera::update_look_at(*camera, mInputSource, deltaTime);
                mScene.SyncCameraTransform(controlled);
            }

        mScene.UpdateCameras(controlled);
    }

    void Engine::UpdateFrameBuffers(FrameData &frame, const RenderTarget &sceneView, const CommandRecorder &recorder)
    {
        UpdateCameraBuffer(frame, sceneView);
        UpdateMeshDataBuffer(frame);
        UpdateMaterialBuffer(frame);
        UpdateSceneBuffer(frame);
        UpdateCullerBuffers(frame, recorder);

        recorder.BufferMemoryBarriers(frame.GetBufferMemoryBarriers());
    }

    void Engine::UpdateCameraBuffer(FrameData &frame, const RenderTarget &sceneView) const
    {
        const auto sceneExtent{sceneView.GetExtent()};

        const auto simulationExtent{
            (mSimulationView.GetExtent().x > 0 && mSimulationView.GetExtent().y > 0)
                ? mSimulationView.GetExtent()
                : sceneExtent
        };

        std::array<CameraData, details::kMaxCameras + details::kMaxCameraViews> cameras{};
        cameras[details::kSceneViewCameraIndex] =
            BuildCameraData(mScene.GetSceneCamera(), sceneExtent);

        if (const auto primary{mScene.GetPrimaryCamera()}; primary != entt::null)
            cameras[details::kSimulationViewCameraIndex] =
                BuildCameraData(primary, simulationExtent);

        frame.GetCameraBuffer().Upload(cameras.data(), sizeof(CameraData) * cameras.size());
    }

    void Engine::UpdateMeshDataBuffer(FrameData &frame) const
    {
        std::vector<MeshData> data;
        auto view = mScene.GetEntityRegistry().view<
            WorldTransform,
            MeshMaterialData,
            MeshComponent,
            MaterialComponent>
        (entt::exclude<PendingUploadComponent>);
        data.reserve(view.size_hint());
        for (auto entity: view)
        {
            const auto &world{view.get<WorldTransform>(entity)};
            const auto &mesh{view.get<MeshComponent>(entity)};
            auto boundingSphere{view.get<MeshComponent>(entity).boundingSphere};
            auto material{view.get<MeshMaterialData>(entity)};
            material.materialIdx = view.get<MaterialComponent>(entity).handle.index;

            const auto lodInfo{mMeshRegistry.GetLodInfo(mesh.handle)};
            const glm::vec4 errors{
                lodInfo.errors[0],
                lodInfo.errors[1],
                lodInfo.errors[2],
                lodInfo.errors[3]
            };

            data.emplace_back(
                world.model,
                boundingSphere,
                errors,
                material,
                static_cast<uint32_t>(entity),
                lodInfo.count
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
        auto data = mScene.GetData();
        for (uint32_t view{}; view < details::kMaxCameraViews; ++view)
        {
            data.directionalShadows.views[view] = mDirectionalShadowSets[view].GetData();
            data.pointShadows.views[view] = mPointShadowSets[view].GetData();
        }
        frame.GetSceneBuffer().Upload(&data, sizeof(SceneData));
    }

    void Engine::UpdateCullerBuffers(const FrameData &frame, const CommandRecorder &recorder)
    {
        std::vector<vk::DrawIndexedIndirectCommand> indirectCommands;
        auto view = mScene.GetEntityRegistry().view<
            WorldTransform,
            MeshMaterialData,
            MeshComponent,
            MaterialComponent>
        (entt::exclude<PendingUploadComponent>);
        indirectCommands.reserve(
            mScene.GetEntityCount<MeshComponent>(entt::exclude<PendingUploadComponent>)
            * details::kMaxGeometryLods
        );
        uint32_t objectIndex{};
        for (const auto entity : view)
        {
            const auto &mesh{view.get<MeshComponent>(entity)};
            for (uint32_t lod{}; lod < details::kMaxGeometryLods; ++lod)
            {
                const auto meshView{mMeshRegistry.View(mesh.handle, lod)};
                indirectCommands.emplace_back(
                    meshView.indexCount,
                    1,
                    meshView.firstIndex,
                    meshView.vertexOffset,
                    objectIndex
                );
            }
            ++objectIndex;
        }
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
        mSwapchain.Recreate(window, mContext, kSceneSamples);
    }

    void Engine::AddPhysicsToEntity(entt::entity entity, PhysicsCreationOptions options)
    {
        if (const auto handle{mPhysicsSystem.AddPhysicsToEntity(entity, options)})
            mScene.AttachPhysics(entity, {*handle, options.bodyType, options.canBecomeDynamic});
    }

    void Engine::AddLightEntity(LightType type)
    {
        switch (type)
        {
            case LightType::Point:
                if (const auto pointLight{
                    mScene.CreatePointLightEntity(
                        mScene.GetLightEntityName(),
                        {mGizmoRegistry.GetBuiltins().pointLight, 0.5f, {1.f, 1.f, 1.f, 1.f}},
                        {})
                }; !pointLight)
                    mOnWarningLog(pointLight.error());
                break;
            default:
                break;
        }
    }

    void Engine::AddCameraEntity(int width, int height)
    {
        const auto camera{
            mScene.CreateCameraEntity(
                mScene.GetCameraEntityName(),
                {mGizmoRegistry.GetBuiltins().camera, 0.5f, {1.f, 1.f, 1.f, 1.f}},
                {},
                false
            )
        };
        if (!camera)
            mOnWarningLog(camera.error());
    }

    DeviceInfo Engine::GetDeviceInfo() const
    {
        return mContext.GetDeviceInfo();
    }

    void Engine::SetRenderScale(float scale)
    {
        scale = std::clamp(scale, kMinRenderScale, 1.f);
        if (std::abs(scale - mRenderScale) < 1e-4f)
            return;

        mRenderScale = scale;

        ResizeSceneView(apply_scale(mSceneViewExtent, mRenderScale));
    }
}
