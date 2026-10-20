#include "Engine.h"

#include <fstream>
#include <magic_enum/magic_enum.hpp>

#include "Clock.h"
#include "EngineSpecification.h"
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
#include "components/entt/NativeScriptComponent.h"
#include "components/entt/PendingUploadComponent.h"
#include "components/entt/PhysicsComponent.h"
#include "components/entt/PhysicsControlComponent.h"
#include "components/entt/SceneSettings.h"
#include "components/entt/TagComponent.h"
#include "components/entt/WorldTransform.h"
#include "components/gpu/CameraData.h"
#include "components/gpu/MeshData.h"
#include "shadow/ShadowCascades.h"
#include "texture/TextureAllocator.h"

namespace kailux
{
    Engine::Engine() : mAssetPipeline(mContext, mMeshRegistry, mTextureRegistry, mTransferManager, mFrames),
                       mPhysicsSystem(mPhysicsRegistry),
                       mCurrentFrame(0),
                       mOutlineInfo(),
                       mPickedEntity(~0u),
                       mOnInfoLog([](auto message) { log::file.Info("{}", message); }),
                       mOnWarningLog([](auto message) { log::file.Warning("{}", message); }),
                       mOnErrorLog([](auto message) { log::file.Error("{}", message); })
    {
    }

    Engine::Engine(Engine &&other) noexcept : mSpecification(other.mSpecification),
                                              mContext(std::move(other.mContext)),
                                              mSwapchain(std::move(other.mSwapchain)),
                                              mImGuiBackend(std::move(other.mImGuiBackend)),
                                              mTransferManager(std::move(other.mTransferManager)),
                                              mMeshRegistry(std::move(other.mMeshRegistry)),
                                              mTextureRegistry(std::move(other.mTextureRegistry)),
                                              mPhysicsRegistry(std::move(other.mPhysicsRegistry)),
                                              mGizmoRegistry(std::move(other.mGizmoRegistry)),
                                              mAssetPipeline(std::move(other.mAssetPipeline)),
                                              mPhysicsSystem(std::move(other.mPhysicsSystem)),
                                              mScriptSystem(std::move(other.mScriptSystem)),
                                              mDeferredResourceEraser(std::move(other.mDeferredResourceEraser)),
                                              mFrames(std::move(other.mFrames)),
                                              mCurrentFrame(other.mCurrentFrame),
                                              mControlledCamera(other.mControlledCamera),
                                              mInputSource(other.mInputSource),
                                              mMouseLookActive(other.mMouseLookActive),
                                              mSimulationStartPending(other.mSimulationStartPending),
                                              mBuiltinCameraControl(other.mBuiltinCameraControl),
                                              mStartSimulationOnAttach(other.mStartSimulationOnAttach),
                                              mScrollDelta(other.mScrollDelta),
                                              pThreadDispatcher(other.pThreadDispatcher),
                                              mOnLoadSceneRequest(std::move(other.mOnLoadSceneRequest)),
                                              mSceneViews(std::move(other.mSceneViews)),
                                              mSimulationViews(std::move(other.mSimulationViews)),
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
        CreateScriptSystem();
    }

    Engine &Engine::operator=(Engine &&other) noexcept
    {
        if (this != &other)
        {
            mSpecification = other.mSpecification;
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
            mScriptSystem = std::move(other.mScriptSystem);
            mDeferredResourceEraser = std::move(other.mDeferredResourceEraser);
            mFrames = std::move(other.mFrames);
            mCurrentFrame = other.mCurrentFrame;
            mControlledCamera = other.mControlledCamera;
            mInputSource = other.mInputSource;
            mMouseLookActive = other.mMouseLookActive;
            mSimulationStartPending = other.mSimulationStartPending;
            mBuiltinCameraControl = other.mBuiltinCameraControl;
            mStartSimulationOnAttach = other.mStartSimulationOnAttach;
            mScrollDelta = other.mScrollDelta;
            pThreadDispatcher = other.pThreadDispatcher;
            mOnLoadSceneRequest = std::move(other.mOnLoadSceneRequest);
            mSceneViews = std::move(other.mSceneViews);
            mSimulationViews = std::move(other.mSimulationViews);
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
            CreateScriptSystem();
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

    Engine Engine::create(Window &window, EngineSpecification specification)
    {
        Engine engine;

        engine.mSpecification = specification;
        if (engine.mSpecification.renderMode == RenderMode::Editor)
            engine.mSpecification.useImgui = true;
        else
            engine.mSimulationViewActive = true;

        engine.CreateRenderingContext(window);
        if (!engine.IsEditor())
            engine.CheckRuntimePresentSupport();

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

        if (engine.UsesImGui())
            engine.CreateImGui(window);
        if (engine.IsEditor())
            engine.AcquireViewTextureIds();

        engine.CreateAssetPipeline();
        engine.CreatePhysicsSystem();
        engine.CreateScriptSystem();

        return engine;
    }

    void Engine::SetOnImGuiRender(OnImGuiRender &&callback)
    {
        mOnImGuiRender = std::move(callback);
    }

    CameraData Engine::GetCameraData(Scene& scene) const
    {
        const auto &sceneView{mSceneViews[mCurrentFrame]};
        return build_camera_data(scene, scene.GetSceneCamera(), sceneView.GetExtent());
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

    void Engine::ToggleMouseLook(Scene &scene)
    {
        if (!mInputSource.Valid() || !mBuiltinCameraControl)
            return;

        mMouseLookActive = !mMouseLookActive;
        mInputSource.SetCursorMode(mMouseLookActive ? CursorMode::Disabled : CursorMode::Normal);

        if (mMouseLookActive)
            if (auto* camera = scene.GetEntityRegistry().try_get<CameraComponent>(mControlledCamera))
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

    void Engine::HandleMeshDragDrop(const std::filesystem::path &path, std::reference_wrapper<ThreadDispatcher> threadDispatcher)
    {
        std::optional<std::string> pathStr;
        if (std::filesystem::is_directory(path))
        {
            if (const auto supported{get_supported_mesh_type(path)})
                pathStr = supported->string();
        }
        else if (is_mesh_type_supported(path))
            pathStr = path.string();

        if (!pathStr)
        {
            mOnWarningLog(std::format("No supported mesh found in '{}'", path.string()));
            return;
        }

        if (mAssetPipeline.IsCached(*pathStr))
            mAssetPipeline.GetPendingQueue().Emplace(
                entt::null,
                std::move(*pathStr),
                mesh_loader::LoadData{},
                "",
                Transform{},
                MeshMaterialData{},
                MeshType::Loaded
            );
        else
            threadDispatcher.get().Enqueue([this, p = *pathStr]()
            {
                if (auto data = mesh_loader::load(p))
                    mAssetPipeline.GetPendingQueue().Emplace(
                        entt::null,
                        std::move(p),
                        std::move(*data),
                        "",
                        Transform{},
                        MeshMaterialData{},
                        MeshType::Loaded
                    );
                else
                    mOnWarningLog(data.error());
            });
    }

    void Engine::UnregisterMesh(Scene &scene, entt::entity entity)
    {
        auto &registry{scene.GetEntityRegistry()};

        auto root{entity};
        while (const auto *hierarchy{registry.try_get<HierarchyComponent>(root)})
        {
            if (hierarchy->parent == entt::null)
                break;
            root = hierarchy->parent;
        }

        const auto *source{registry.try_get<MeshSourceComponent>(root)};
        if (!source)
            return;

        const auto model{mAssetPipeline.Uncache(source->path)};
        if (!model)
            return;

        for (const auto &submesh : model->submeshes)
        {
            mMeshRegistry.Destroy(submesh.meshHandle);
            const auto materialHandle{submesh.materialHandle};
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

    glm::ivec2 Engine::GetSceneViewExtent() const
    {
        return mSceneViewExtent;
    }

    ImTextureID Engine::GetSimulationTextureId() const
    {
        return GetCurrentSimulationView().GetTextureId();
    }

    void Engine::OnEvent(Scene &scene, const Event &event, Window &window)
    {
        if (const auto *scrolled{std::get_if<MouseScrolled>(&event)})
            mScrollDelta += glm::vec2{
                static_cast<float>(scrolled->xOffset),
                static_cast<float>(scrolled->yOffset)
            };

        if (const auto* keyReleased{std::get_if<KeyReleased>(&event)})
        {
            const auto key{keyReleased->key};
            if (key == Key::Escape && mMouseLookActive)
                ToggleMouseLook(scene);
            else if (key == Key::Tab)
                scene.SetPrimaryCamera(scene.GetNextCamera(scene.GetPrimaryCamera()));
        }
    }

    void Engine::StartSimulation(Scene &scene)
    {
        mSimulationStartPending = false;
        scene.UpdateCameras();
        mPhysicsSystem.SetSimulationState(scene, SimulationState::Running);
        mScriptSystem.SetFixedStep(1.f / mPhysicsSystem.GetTickRate());
        mScriptSystem.SetSimulationState(scene, SimulationState::Running);
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
        mAssetPipeline = {mContext, mMeshRegistry, mTextureRegistry, mTransferManager, mFrames};
        mAssetPipeline.SetOnInfoLog([this](auto msg)
        {
            mOnInfoLog(msg);
        });
        mAssetPipeline.SetOnWarningLog([this](auto msg)
        {
            mOnWarningLog(msg);
        });
        mAssetPipeline.SetOnAttachPhysics([this](auto& scene, auto entity, auto physicsRecord)
        {
            AddPhysicsToEntity(
                scene,
                entity,
                {
                    physicsRecord.type,
                    physicsRecord.canBecomeDynamic,
                    physicsRecord.mass,
                    physicsRecord.isSensor
                }
            );
        });
    }

    void Engine::CreatePhysicsSystem()
    {
        mPhysicsSystem = PhysicsSystem{mPhysicsRegistry};
        mPhysicsSystem.SetOnInfoLog([this](auto msg)
        {
            mOnInfoLog(msg);
        });
        mPhysicsSystem.SetOnWarningLog([this](auto msg)
        {
            mOnWarningLog(msg);
        });
        mPhysicsSystem.SetOnFixedStep([this](auto& scene, float step)
        {
           mScriptSystem.FixedUpdate(scene, step);
        });
    }

    void Engine::CreateScriptSystem()
    {
        mScriptSystem = {};
        mScriptSystem.SetServices({
            [this](Scene& scene, std::string_view name, MeshType type, const Transform& transform)
            {
                return SpawnPrimitive(scene, name, type, transform);
            },
            [this](Scene& scene, const std::filesystem::path& path, std::string_view name, const Transform& transform,
                   std::optional<PhysicsCreationOptions> physics)
            {
                return SpawnMesh(scene, path, name, transform, physics);
            },
            [this](Scene& scene, entt::entity entity, PhysicsCreationOptions options)
            {
                return AddPhysicsToEntity(scene, entity, options);
            },
            [this](const std::filesystem::path& path)
            {
                if (!mOnLoadSceneRequest || !mOnLoadSceneRequest(path))
                    return false;
                mStartSimulationOnAttach = true;
                return true;
            },
            [this](bool enabled)
            {
                SetBuiltinCameraControl(enabled);
            }
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

        const auto sceneViewExtent{IsEditor() ? initialExtent : glm::ivec2{1, 1}};

        for (auto &view : mSceneViews)
            view = RenderTarget::create(mContext, {
                mSwapchain.GetFormat(),
                mSwapchain.GetDepthFormat(),
                sceneViewExtent,
                kSceneSamples,
                true
            });

        mSceneViewExtent = sceneViewExtent;

        for (auto &view: mSimulationViews)
            view = RenderTarget::create(mContext,
                                        {
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

        for (auto &view : mSimulationViews)
            view.SetTextureId(
                ImGuiBackend::get_texture_id_from_texture(view.GetPresentedTexture()));
    }

    void Engine::CheckRuntimePresentSupport() const
    {
        constexpr vk::FormatFeatureFlags kRequired{
            vk::FormatFeatureFlagBits::eBlitSrc |
            vk::FormatFeatureFlagBits::eBlitDst |
            vk::FormatFeatureFlagBits::eSampledImageFilterLinear
        };

        const auto features{
            mContext.GetPhysicalDevice().getFormatProperties(mSwapchain.GetFormat()).optimalTilingFeatures
        };
        if ((features & kRequired) != kRequired)
            throw std::runtime_error("Runtime mode: the swapchain format does not support blitting");

        const auto capabilities{
            mContext.GetPhysicalDevice().getSurfaceCapabilitiesKHR(mContext.GetSurface())
        };
        if (!(capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst))
            throw std::runtime_error("Runtime mode: swapchain images cannot be used as a transfer destination");
    }

    bool Engine::IsEditor() const
    {
        return mSpecification.renderMode == RenderMode::Editor;
    }

    bool Engine::UsesImGui() const
    {
        return mSpecification.useImgui;
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
            vk::PipelineStageFlagBits2::eAllCommands
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

    void Engine::Render(Scene &scene, const Window &window)
    {
        auto &frame{mFrames[mCurrentFrame]};
        const auto &sceneView{mSceneViews[mCurrentFrame]};

        frame.Reset(mContext);
        if (IsEditor())
            ReadOutputBuffers(frame);

        const auto acquired{mSwapchain.Acquire()};
        if (!acquired)
        {
            RecreateSwapchainResources(window);
            return;
        }

        if (!IsEditor())
            SetSimulationViewExtent(apply_scale({
                                                    static_cast<int>(mSwapchain.GetExtent().width),
                                                    static_cast<int>(mSwapchain.GetExtent().height)
                                                },
                                                mRenderScale
                )
            );

        if (const auto extent{mSceneResize.Poll(sceneView.GetExtent())})
            ResizeSceneView(*extent);

        if (mSimulationViewActive)
            if (const auto extent{mSimulationResize.Poll(GetCurrentSimulationView().GetExtent())})
                ResizeSimulationView(*extent);

        const auto sceneExtent{sceneView.GetVkExtent()};
        const auto sceneCamera{IsEditor() ? scene.GetSceneCamera() : entt::null};

        const auto renderFinishedSemaphore = mSwapchain.GetPresentSemaphore(acquired->imageIndex); {
            CommandRecorder recorder(frame.GetCommandBuffer());

            mDirectionalShadowSets[details::kSceneViewCameraIndex].Update(
               scene,
               sceneCamera,
               mSceneViews[mCurrentFrame].GetExtent()
               );
            mDirectionalShadowSets[details::kSimulationViewCameraIndex].Update(
                scene,
                mSimulationViewActive ? scene.GetPrimaryCamera() : entt::null,
                GetCurrentSimulationView().GetExtent()
            );
            mPointShadowSets[details::kSceneViewCameraIndex].Update(scene, sceneCamera);
            mPointShadowSets[details::kSimulationViewCameraIndex].Update(
                scene,
                mSimulationViewActive ? scene.GetPrimaryCamera() : entt::null
            );

            UpdateFrameBuffers(scene, frame, sceneView, recorder);
            if (IsEditor())
                ExecuteCulling(
                    scene,
                    frame,
                    recorder,
                    sceneCamera,
                    sceneExtent,
                    CullingPreset::SceneView
                    );

            TransitionForShadowPass(frame, recorder);
            for (uint32_t view{}; view < details::kMaxCameraViews; ++view)
                RecordDirectionalShadows(scene, frame, recorder, view);
            TransitionShadowMapForSampling(frame, recorder);

            TransitionForPointShadowPass(frame, recorder);
            for (uint32_t view{}; view < details::kMaxCameraViews; ++view)
                RecordPointShadows(scene, frame, recorder, view);
            TransitionPointShadowMapForSampling(frame, recorder);

            if (IsEditor())
            {
                RenderScenePass(frame, recorder, sceneView);
                RecordEditorPasses(scene, frame, recorder, sceneView, acquired->imageIndex);
            }
            else
                RecordRuntimePresent(scene, frame, recorder, acquired->imageIndex);
        }

        Submit(mFrames[mCurrentFrame], acquired->imageAvailableSemaphore, renderFinishedSemaphore);

        if (UsesImGui())
            mImGuiBackend.UpdatePlatform();

        if (!mSwapchain.Present(mContext, acquired->imageIndex, renderFinishedSemaphore))
            RecreateSwapchainResources(window);

        RetireViews();

        mCurrentFrame = (mCurrentFrame + 1) % details::kFramesInFlight;
    }

    std::optional<std::filesystem::path> Engine::get_supported_mesh_type(const std::filesystem::path &directory)
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator{directory})
            if (entry.is_regular_file() && is_mesh_type_supported(entry.path()))
                return entry.path();

        return std::nullopt;
    }

    bool Engine::is_mesh_type_supported(const std::filesystem::path &path)
    {
        using namespace std::string_view_literals;
        static constexpr std::array supported =
        {
            "fbx"sv,
            "gltf"sv,
            "obj"sv
        };

        return std::ranges::contains(supported, get_extension(path));
    }

    bool Engine::is_image_type_supported(const std::filesystem::path &path)
    {
        using namespace std::string_view_literals;
        static constexpr std::array supported =
        {
            "jpeg"sv,
            "jpg"sv,
            "png"sv
        };

        return std::ranges::contains(supported, get_extension(path));
    }

    std::string Engine::get_extension(const std::filesystem::path &path)
    {
        auto extension{path.extension().string()};
        if (!extension.empty())
            extension.erase(0, 1);

        std::ranges::transform(extension, extension.begin(),
                               [](unsigned char c)
                               {
                                   return static_cast<char>(std::tolower(c));
                               });

        return extension;
    }

    void Engine::AttachScene(Scene &scene, std::span<const MeshRequest> meshRequests)
    {
        const bool startSimulation{std::exchange(mStartSimulationOnAttach, false)};

        mPhysicsSystem.AttachToScene(scene);
        ScriptSystem::connect(scene);

        for (const auto &request : meshRequests)
        {
            AssetPipeline::PendingMeshData pending;
            pending.target    = request.target;
            pending.name      = request.name;
            pending.path      = request.record.path;
            pending.type      = request.record.type;
            pending.transform = request.transform;
            pending.material  = request.material;
            pending.physics   = request.physics;
            pending.scene     = scene.GetId();

            if (!mAssetPipeline.IsCached(pending.path))
                if (auto data = mesh_loader::load(pending.path))
                    pending.data = std::move(*data);

            mAssetPipeline.GetPendingQueue().Push(std::move(pending));
        }

        if (startSimulation)
            RequestSimulationState(scene, SimulationState::Running);
    }

    void Engine::DetachScene(Scene &scene)
    {
        mSimulationStartPending = false;
        mBuiltinCameraControl = true;
        mPhysicsSystem.SetSimulationState(scene, SimulationState::Paused);
        mScriptSystem.SetSimulationState(scene, SimulationState::Paused);
        mPhysicsSystem.DestroyAllBodies();
        SetControlledCamera(entt::null, {});
    }

    SceneLoadContext Engine::GetSceneLoadContext(const Window &window)
    {
        const auto fbSize{window.GetInputSource().GetFramebufferSize()};
        return {mGizmoRegistry, fbSize.x, fbSize.y};
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

    void Engine::SetSelectedEntity(uint32_t entity)
    {
        mOutlineInfo.id = entity;
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

    void Engine::UpdateBodyMass(BodyHandle handle, float mass)
    {
        mPhysicsRegistry.SetBodyMass(handle, mass);
    }

    bool Engine::RequestSimulationState(Scene &scene, SimulationState state)
    {
        if (state == SimulationState::Paused)
        {
            mSimulationStartPending = false;
            mPhysicsSystem.SetSimulationState(scene, state);
            mScriptSystem.SetSimulationState(scene, state);
            return true;
        }

        if (scene.GetPrimaryCamera() == entt::null)
        {
            mOnWarningLog("Cannot start simulation: the scene has no camera");
            return false;
        }
        if (!IsSceneReady(scene))
        {
            mSimulationStartPending = true;
            mOnInfoLog("Simulation start deferred until all scene assets are loaded");
            return true;
        }

        StartSimulation(scene);
        return true;
    }

    bool Engine::IsSceneReady(const Scene &scene) const
    {
        return mAssetPipeline.IsIdle()
               && !mTransferManager.HasPending()
               && scene.GetEntityCount<PendingUploadComponent>() == 0u;
    }

    entt::entity Engine::SpawnPrimitive(Scene &scene, std::string_view name, MeshType type, const Transform &transform)
    {
        static const auto kCubeBounds{Geometry::compute_bounding_sphere(mesh_geometry::generate_cube().vertices)};
        static const auto kSphereBounds{Geometry::compute_bounding_sphere(mesh_geometry::generate_sphere().vertices)};

        if (type != MeshType::Cube && type != MeshType::Sphere)
        {
            mOnWarningLog("SpawnPrimitive only supports cubes and spheres");
            return entt::null;
        }

        const auto& builtins{mMeshRegistry.GetBuiltins()};
        const MeshComponent component{
            type == MeshType::Cube ? builtins.cube : builtins.sphere,
            type == MeshType::Cube ? kCubeBounds : kSphereBounds
        };
        const auto created{scene.CreateMeshEntity(
            name.empty() ? scene.GetMeshEntityName() : std::string{name},
            component,
            mTextureRegistry.GetDefaultMaterialHandle(),
            transform,
            MeshMaterialData{}
        )};
        if (!created)
        {
            mOnWarningLog(created.error());
            return entt::null;
        }
        scene.AttachMeshSource(*created, {"", type});
        return *created;
    }

    entt::entity Engine::SpawnMesh(Scene &scene, const std::filesystem::path &path, std::string_view name,
        const Transform &transform, std::optional<PhysicsCreationOptions> physics)
    {
        if (!is_mesh_type_supported(path))
        {
            mOnWarningLog(std::format("SpawnMesh: '{}' is not a supported mesh file", path.string()));
            return entt::null;
        }

        const auto entity{scene.CreateEntity(name.empty() ? scene.GetMeshEntityName() : std::string{name})};
        scene.SetLocalTransform(entity, transform);

        AssetPipeline::PendingMeshData pending;
        pending.target    = entity;
        pending.name      = std::string{name};
        pending.path      = path.string();
        pending.type      = MeshType::Loaded;
        pending.transform = transform;
        pending.scene     = scene.GetId();

        if (physics)
            pending.physics = {
                physics->bodyType,
                physics->canBecomeDynamic,
                physics->mass,
                physics->isSensor
            };

        if (mAssetPipeline.IsCached(pending.path) || !pThreadDispatcher)
        {
            if (!mAssetPipeline.IsCached(pending.path))
            {
                auto data{mesh_loader::load(pending.path)};
                if (!data)
                {
                    mOnWarningLog(data.error());
                    return entity;
                }
                pending.data = std::move(*data);
            }
            mAssetPipeline.GetPendingQueue().Push(std::move(pending));
            return entity;
        }

        pThreadDispatcher->Enqueue([this, pending = std::move(pending)]() mutable
        {
            auto data{mesh_loader::load(pending.path)};
            if (!data)
            {
                mOnWarningLog(data.error());
                return;
            }
            pending.data = std::move(*data);
            mAssetPipeline.GetPendingQueue().Push(std::move(pending));
        });
        return entity;
    }

    void Engine::ExecuteCulling(
        const Scene &scene,
        const FrameData &frame,
        const CommandRecorder &recorder,
        entt::entity camera,
        vk::Extent2D extent,
        CullingPreset preset
    ) const
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

        const auto totalObjects{scene.GetEntityCount<MeshComponent>(entt::exclude<PendingUploadComponent>)};
        if (totalObjects == 0)
            return;

        mComputeCuller.Bind(cmd);
        frame.GetCullerDescriptorSet().Bind(mComputeCuller.GetPipeline(), cmd);

        const auto cameraData{build_camera_data(scene, camera, {extent.width, extent.height})};

        const auto planes{camera::get_frustum_planes(cameraData.projection, cameraData.view)};

        const float projFactor{
            0.5f * static_cast<float>(extent.height) * std::abs(cameraData.projection[1][1])
        };
        const glm::vec4 cameraPosition{glm::vec3(cameraData.positionAndExposure), projFactor};

        const auto& settings{scene.GetEntityRegistry().get<SceneSettings>(scene.GetSettingsEntity())};

        mComputeCuller.Push<compute_pass_push_constants::CullParams>(
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
            if (IsEditor())
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

        for (auto &view : mSimulationViews)
        {
            if (view.GetTextureId())
                mRetiredViews.push_back(std::move(view));

            view = RenderTarget::create(mContext, {
                    mSwapchain.GetFormat(),
                    mSwapchain.GetDepthFormat(),
                    extent,
                    mSimulationSamples,
                    false
                });

            if (IsEditor())
                view.SetTextureId(
                    ImGuiBackend::get_texture_id_from_texture(view.GetPresentedTexture()));
        }
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
        const auto& simulationView{GetCurrentSimulationView()};
        recorder.ApplyImageBarrier({
            simulationView.GetColorTexture().GetImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eColorAttachmentWrite
        });

        recorder.ApplyImageBarrier({
            simulationView.GetDepthTexture().GetImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests,
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::ImageAspectFlagBits::eDepth
        });

        if (simulationView.IsMultisampled())
            recorder.ApplyImageBarrier({
                simulationView.GetPresentedTexture().GetImage(),
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

    void Engine::TransitionForBlit(const CommandRecorder &recorder,uint32_t imageIndex, bool hasSource) const
    {
        if (hasSource)
            recorder.ApplyImageBarrier({
                GetCurrentSimulationView().GetPresentedTexture().GetImage(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eTransferSrcOptimal,
                vk::PipelineStageFlagBits2::eFragmentShader,
                vk::PipelineStageFlagBits2::eBlit,
                vk::AccessFlagBits2::eNone,
                vk::AccessFlagBits2::eTransferRead
            });

        recorder.ApplyImageBarrier({
            mSwapchain.GetImage(imageIndex),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eAllTransfer,
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eTransferWrite
        });
    }

    void Engine::TransitionAfterBlit(const CommandRecorder &recorder, uint32_t imageIndex) const
    {
        const bool overlay{UsesImGui()};

        recorder.ApplyImageBarrier({
            mSwapchain.GetImage(imageIndex),
            vk::ImageLayout::eTransferDstOptimal,
            overlay ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits2::eAllTransfer,
            overlay ? vk::PipelineStageFlagBits2::eColorAttachmentOutput : vk::PipelineStageFlagBits2::eBottomOfPipe,
            vk::AccessFlagBits2::eTransferWrite,
            overlay
                ? vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite
                : vk::AccessFlagBits2::eNone
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

    void Engine::RecordDirectionalShadows(const Scene &scene, const FrameData &frame, CommandRecorder &recorder, uint32_t viewIndex) const
    {
        const auto cmd{recorder.GetCommandBuffer()};
        const auto extent{mDirectionalShadowMap.GetExtent()};

        const auto objectCount{scene.GetEntityCount<MeshComponent>(entt::exclude<PendingUploadComponent>)};
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
                mShadowPass.Push(cmd, graphics_pass_push_constants::ShadowCascade{
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

    void Engine::RecordPointShadows(const Scene &scene, const FrameData &frame, CommandRecorder &recorder, uint32_t viewIndex) const
    {
        const auto cmd{recorder.GetCommandBuffer()};
        const auto extent{mPointShadowMap.GetExtent()};
        const auto &shadowSet{mPointShadowSets[viewIndex]};

        const auto objectCount{scene.GetEntityCount<MeshComponent>(entt::exclude<PendingUploadComponent>)};

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
                mShadowPass.Push(cmd, graphics_pass_push_constants::ShadowCascade{
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

    void Engine::RecordGizmos(const Scene &scene, const FrameData &frame, const CommandRecorder &recorder) const
    {
        const auto cmd = recorder.GetCommandBuffer();
        mGizmoPass.Bind(cmd);
        mGizmoRegistry.Bind(cmd);
        frame.GetGizmoDescriptorSet().Bind(mGizmoPass.GetPipeline(), cmd);

        const auto view{scene.GetEntityRegistry().view<GizmoComponent, WorldTransform>()};
        view.each([&](const auto& component, const auto& world)
        {
            auto gizmoView = mGizmoRegistry.View(component.handle);

            const graphics_pass_push_constants::Gizmo pc{
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

    void Engine::RecordImGuiData(Scene &scene, const FrameData &frame)
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
        if (mOnImGuiRender)
            mOnImGuiRender(scene);
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

    void Engine::RecordOutline(const Scene &scene, const FrameData &frame, const CommandRecorder &recorder)
    {
        const auto cmd = recorder.GetCommandBuffer();
        mOutlinePass.Bind(cmd);
        frame.GetOutlineDescriptorSet().Bind(mOutlinePass.GetPipeline(), cmd);
        mOutlineInfo.color = {scene.GetEntityRegistry().get<SceneSettings>(scene.GetSettingsEntity()).outlineColor, 0.f};
        mOutlinePass.Push(cmd, mOutlineInfo);
        cmd.draw(3, 1, 0, 0);
    }

    void Engine::RenderSimulationView(const Scene &scene, const FrameData &frame, CommandRecorder &recorder)
    {
        const auto primaryCamera{scene.GetPrimaryCamera()};
        if (primaryCamera == entt::null)
            return;

        const auto& simulationView{GetCurrentSimulationView()};

        const vk::Extent2D extent{
            static_cast<uint32_t>(simulationView.GetExtent().x),
            static_cast<uint32_t>(simulationView.GetExtent().y)
        };

        recorder.BufferMemoryBarriers(frame.GetIndirectReadToWriteBarriers());
        ExecuteCulling(scene, frame, recorder, primaryCamera, extent, CullingPreset::SimulationView);

        TransitionForSimulationPass(recorder);

        const std::array attachments{
            ColorAttachmentInfo{
                simulationView.GetColorTexture().GetImageView(),
                simulationView.GetResolveView(),
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::ClearColorValue{std::array{0.f, 0.f, 0.f, 1.f}},
                simulationView.IsMultisampled()
                    ? vk::ResolveModeFlagBits::eAverage
                    : vk::ResolveModeFlagBits::eNone
            }
        };

        recorder.BeginRendering({
            attachments,
            extent,
            simulationView.GetDepthTexture().GetImageView(),
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::AttachmentLoadOp::eClear
        });

        recorder.SetViewport(extent);
        recorder.SetScissor(extent);

        RecordMeshData(frame, recorder, details::kSimulationViewCameraIndex, false);
        RecordSkybox(frame, recorder, details::kSimulationViewCameraIndex, simulationView.IsMultisampled());

        recorder.EndRendering();

        recorder.ApplyImageBarrier({
            simulationView.GetPresentedTexture().GetImage(),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eShaderRead
        });
    }

    void Engine::RecordEditorPasses(Scene &scene, FrameData &frame, CommandRecorder &recorder, const RenderTarget &sceneView,
                                    uint32_t imageIndex)
    {
        const auto sceneExtent{sceneView.GetVkExtent()};
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

        RecordGizmos(scene, frame, recorder);
        recorder.EndRendering();

        TransitionForOutlinePass(sceneView, recorder, imageIndex);
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

        RecordOutline(scene, frame, recorder);
        recorder.EndRendering();

        TransitionForPickerAndPostProcess(frame, sceneView, recorder);
        RecordPicker(frame, recorder);

        const std::array pickerMemBarrier{frame.GetPickerBufferMemoryBarrier()};
        recorder.BufferMemoryBarriers(pickerMemBarrier);
        if (mSimulationViewActive &&
            GetCurrentSimulationView().GetExtent().x > 0 &&
            GetCurrentSimulationView().GetExtent().y > 0
        )
            RenderSimulationView(scene, frame, recorder);

        RecordImGuiPass(scene, frame, recorder, imageIndex);
        TransitionForPresent(recorder, imageIndex);
    }

    void Engine::RecordRuntimePresent(Scene &scene, const FrameData &frame, CommandRecorder &recorder, uint32_t imageIndex)
    {
        const bool hasView{
            scene.GetPrimaryCamera() != entt::null &&
            GetCurrentSimulationView().GetExtent().x > 0 &&
            GetCurrentSimulationView().GetExtent().y > 0
        };

        if (hasView)
            RenderSimulationView(scene, frame, recorder);

        TransitionForBlit(recorder, imageIndex, hasView);
        const auto swapchainImage{mSwapchain.GetImage(imageIndex)};
        if (hasView)
        {
            const auto sourceExtent{GetCurrentSimulationView().GetVkExtent()};
            const auto targetExtent{mSwapchain.GetExtent()};
            constexpr vk::ImageSubresourceLayers kColorLayer{vk::ImageAspectFlagBits::eColor, 0, 0, 1};
            const vk::ImageBlit2 region{
                kColorLayer,
                std::array{
                    vk::Offset3D{0, 0, 0},
                    vk::Offset3D{static_cast<int32_t>(sourceExtent.width), static_cast<int32_t>(sourceExtent.height), 1}
                },
                kColorLayer,
                std::array{
                    vk::Offset3D{0, 0, 0},
                    vk::Offset3D{static_cast<int32_t>(targetExtent.width), static_cast<int32_t>(targetExtent.height), 1}
                }
            };
            const bool sameSize{
                sourceExtent.width == targetExtent.width &&
                sourceExtent.height == targetExtent.height
            };
            const vk::BlitImageInfo2 blitInfo{
                GetCurrentSimulationView().GetPresentedTexture().GetImage(),
                vk::ImageLayout::eTransferSrcOptimal,
                swapchainImage,
                vk::ImageLayout::eTransferDstOptimal,
                1,
                &region,
                sameSize ? vk::Filter::eNearest : vk::Filter::eLinear
            };
            recorder.GetCommandBuffer().blitImage2(blitInfo);
        }
        else
        {
            constexpr vk::ClearColorValue kBlack{std::array{0.f, 0.f, 0.f, 1.f}};
            constexpr vk::ImageSubresourceRange kColorRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
            recorder.GetCommandBuffer().clearColorImage(
                swapchainImage,
                vk::ImageLayout::eTransferDstOptimal,
                kBlack,
                kColorRange
            );
        }

        TransitionAfterBlit(recorder, imageIndex);
        if (UsesImGui())
        {
            RecordImGuiPass(scene, frame, recorder, imageIndex);
            TransitionForPresent(recorder, imageIndex);
        }
    }

    void Engine::RenderScenePass(const FrameData &frame, CommandRecorder &recorder, const RenderTarget &sceneView)
    {
        const auto sceneExtent{sceneView.GetVkExtent()};

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
    }

    void Engine::RecordImGuiPass(Scene &scene, const FrameData &frame, CommandRecorder &recorder, uint32_t imageIndex)
    {
        const std::array imguiOverlay{
            ColorAttachmentInfo
            (mSwapchain.GetImageView(imageIndex),
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
        RecordImGuiData(scene, frame);
        recorder.GetCommandBuffer().executeCommands(frame.GetImGuiCommandBuffer());
        recorder.EndRendering();
    }

    CameraData Engine::build_camera_data(const Scene &scene, entt::entity entity, glm::ivec2 extent)
    {
        const auto& camera{scene.GetEntityRegistry().get<CameraComponent>(entity)};
        return {
            camera::get_projection(camera, extent.x, extent.y),
            camera::get_view(camera),
            {
                camera.position,
                camera.exposure
            }
        };
    }

    void Engine::Update(Scene &scene, float deltaTime)
    {
        mAssetPipeline.Poll(scene);
        mTransferManager.Poll(mContext);
        mAssetPipeline.FlushCompletedUploads(scene);
        mDeferredResourceEraser.Tick();

        if (mSimulationStartPending && IsSceneReady(scene))
        {
            StartSimulation(scene);
            mOnInfoLog("All scene assets loaded, simulation started");
        }

        const bool scriptsHaveInput{
            mControlledCamera != entt::null
            && mControlledCamera != scene.GetSceneCamera()
            && !mMouseLookActive
        };
        mScriptSystem.BeginFrame(
            deltaTime,
            scriptsHaveInput ? mInputSource : InputSource{},
            std::exchange(mScrollDelta, glm::vec2{0.f})
        );
        
        if (mPhysicsSystem.GetSimulationState() == SimulationState::Running)
        {
            mPhysicsSystem.Update(scene, deltaTime);
            mScriptSystem.DispatchContacts(scene, mPhysicsSystem.DrainContactEvents());
        }

        mScriptSystem.Update(scene, deltaTime);

        scene.Update();

        if (!mInputSource.Valid() || !mBuiltinCameraControl)
        {
            scene.UpdateCameras();
            return;
        }

        const auto controlled{
            mControlledCamera == entt::null ?
            scene.GetSceneCamera() :
            mControlledCamera
        };

        auto& registry{scene.GetEntityRegistry()};
        if (registry.valid(controlled))
            if (auto* camera{registry.try_get<CameraComponent>(controlled)})
            {
                camera->focused = mMouseLookActive;
                camera::update_movement(*camera, mInputSource, deltaTime);
                camera::update_look_at(*camera, mInputSource, deltaTime);
                scene.SyncCameraTransform(controlled);
            }

        scene.UpdateCameras(controlled);
    }

    void Engine::UpdateFrameBuffers(const Scene &scene, FrameData &frame, const RenderTarget &sceneView, const CommandRecorder &recorder)
    {
        UpdateCameraBuffer(scene, frame, sceneView);
        UpdateMeshDataBuffer(scene, frame);
        UpdateMaterialBuffer(frame);
        UpdateSceneBuffer(scene, frame);
        UpdateCullerBuffers(scene, frame, recorder);

        recorder.BufferMemoryBarriers(frame.GetBufferMemoryBarriers());
    }

    void Engine::UpdateCameraBuffer(const Scene &scene, FrameData &frame, const RenderTarget &sceneView) const
    {
        const auto sceneExtent{sceneView.GetExtent()};
        const auto& simulationView{GetCurrentSimulationView()};

        const auto simulationExtent{
            (simulationView.GetExtent().x > 0 && simulationView.GetExtent().y > 0)
                ? simulationView.GetExtent()
                : sceneExtent
        };

        std::array<CameraData, details::kMaxCameras + details::kMaxCameraViews> cameras{};
        cameras[details::kSceneViewCameraIndex] =
            build_camera_data(scene, scene.GetSceneCamera(), sceneExtent);

        if (const auto primary{scene.GetPrimaryCamera()}; primary != entt::null)
            cameras[details::kSimulationViewCameraIndex] =
                build_camera_data(scene, primary, simulationExtent);

        frame.GetCameraBuffer().Upload(cameras.data(), sizeof(CameraData) * cameras.size());
    }

    void Engine::UpdateMeshDataBuffer(const Scene &scene, FrameData &frame) const
    {
        std::vector<MeshData> data;
        auto view = scene.GetEntityRegistry().view<
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

    void Engine::UpdateSceneBuffer(const Scene &scene, FrameData &frame) const
    {
        auto data = scene.GetData();
        for (uint32_t view{}; view < details::kMaxCameraViews; ++view)
        {
            data.directionalShadows.views[view] = mDirectionalShadowSets[view].GetData();
            data.pointShadows.views[view] = mPointShadowSets[view].GetData();
        }
        frame.GetSceneBuffer().Upload(&data, sizeof(SceneData));
    }

    void Engine::UpdateCullerBuffers(const Scene &scene, const FrameData &frame, const CommandRecorder &recorder)
    {
        std::vector<vk::DrawIndexedIndirectCommand> indirectCommands;
        auto view = scene.GetEntityRegistry().view<
            WorldTransform,
            MeshMaterialData,
            MeshComponent,
            MaterialComponent>
        (entt::exclude<PendingUploadComponent>);
        indirectCommands.reserve(
            scene.GetEntityCount<MeshComponent>(entt::exclude<PendingUploadComponent>)
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

    bool Engine::AddPhysicsToEntity(Scene& scene, entt::entity entity, PhysicsCreationOptions options)
    {
        const auto handle{mPhysicsSystem.AddPhysicsToEntity(scene, entity, options)};
        if (!handle)
            return false;

        scene.AttachPhysics(
            entity,
            {
                *handle,
                options.bodyType,
                options.canBecomeDynamic,
                options.mass,
                options.isSensor
            }
        );
        return true;
    }

    void Engine::AddLightEntity(Scene& scene, LightType type)
    {
        switch (type)
        {
            case LightType::Point:
                if (const auto pointLight{
                    scene.CreatePointLightEntity(
                        scene.GetLightEntityName(),
                        {mGizmoRegistry.GetBuiltins().pointLight, 0.5f, {1.f, 1.f, 1.f, 1.f}},
                        {})
                }; !pointLight)
                    mOnWarningLog(pointLight.error());
                break;
            default:
                break;
        }
    }

    void Engine::AddCameraEntity(Scene& scene, int width, int height)
    {
        const auto camera{
            scene.CreateCameraEntity(
                scene.GetCameraEntityName(),
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

        mSceneResize.Request(apply_scale(mSceneViewExtent, mRenderScale));
    }

    void Engine::SetThreadDispatcher(ThreadDispatcher &dispatcher)
    {
        pThreadDispatcher = std::addressof(dispatcher);
    }

    void Engine::SetOnLoadSceneRequest(OnLoadSceneRequest &&callback)
    {
        mOnLoadSceneRequest = std::move(callback);
    }

    void Engine::SetBuiltinCameraControl(bool enabled)
    {
        if (!enabled && mMouseLookActive)
        {
            if (mInputSource.Valid())
                mInputSource.SetCursorMode(CursorMode::Normal);
            mMouseLookActive = false;
        }
        mBuiltinCameraControl = enabled;
    }

    bool Engine::GetBuiltinCameraControl() const
    {
        return mBuiltinCameraControl;
    }
}
