#pragma once

#include "Context.h"
#include "descriptor/DescriptorLayout.h"
#include "Swapchain.h"
#include "FrameData.h"
#include "imgui_backend/ImGuiBackend.h"
#include "window/Event.h"

#include "mesh/MeshRegistry.h"
#include <entt/entt.hpp>

#include "AssetPipeline.h"
#include "passes/ComputePicker.h"
#include "scene/Scene.h"
#include "scene/SceneInstantiator.h"
#include "TransferManager.h"
#include "passes/MainPass.h"
#include "physics/PhysicsRegistry.h"
#include "physics/PhysicsSystem.h"
#include "utilities/Queue.h"
#include "utilities/ThreadDispatcher.h"
#include "DeferredResourceEraser.h"
#include "EngineSpecification.h"
#include "RenderTarget.h"
#include "ResizeDebouncer.h"
#include "audio/AudioSystem.h"
#include "components/gpu/CameraData.h"
#include "gizmo/GizmoRegistry.h"
#include "passes/GizmoPass.h"
#include "scripting/ScriptSystem.h"
#include "shadow/DirectionalShadowSet.h"
#include "shadow/PointShadowSet.h"

namespace kailux
{
    class Engine
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Engine)
        ~Engine();

        static Engine create(Window& window, EngineSpecification specification);

        using OnImGuiRender = std::move_only_function<void(Scene&)>;
        void SetOnImGuiRender(OnImGuiRender&& callback);

        CameraData GetCameraData(Scene& scene) const;

        void SetSceneViewExtent(glm::ivec2 extent);
        void SetSimulationViewExtent(glm::ivec2 extent);
        void SetSimulationViewActive(bool active);
        void SetControlledCamera(entt::entity camera, InputSource source);
        void ToggleMouseLook(Scene &scene);

        void WaitIdle() const;

        Queue<AssetPipeline::PendingMeshData> &GetPendingMeshDataQueue();

        void HandleMeshDragDrop(const std::filesystem::path& path, std::reference_wrapper<ThreadDispatcher> threadDispatcher);

        void UnregisterMesh(Scene &scene, entt::entity entityh);
        void UnregisterMaterial(MaterialHandle handle);

        ImTextureID GetAssetBrowserDirectoryTextureId() const;
        ImTextureID GetAssetBrowserFileTextureId() const;
        ImTextureID GetSceneTextureId() const;
        glm::ivec2  GetSceneViewExtent() const;
        ImTextureID GetSimulationTextureId() const;

        void OnEvent(Scene &scene, const Event& event, Window& window);
        void Update(Scene &scene, float deltaTime);
        void Render(Scene &scene, const Window &window);

        static std::optional<std::filesystem::path> get_supported_mesh_type(const std::filesystem::path &directory);

        static bool is_mesh_type_supported(const std::filesystem::path &path);
        static bool is_image_type_supported(const std::filesystem::path &path);

        static std::string get_extension(const std::filesystem::path &path);

        void AttachScene(Scene &scene, std::span<const MeshRequest> meshRequests);
        void DetachScene(Scene &scene);

        SceneLoadContext GetSceneLoadContext(const Window &window);

        decltype(auto) GetPhysicsSystem(this auto&& self)
        {
            return (self.mPhysicsSystem);
        }

        decltype(auto) GetAudioSystem(this auto&& self)
        {
            return (self.mAudioSystem);
        }

        using OnLog = std::move_only_function<void(std::string_view)>;
        void SetOnInfoLog(OnLog&& callback);
        void SetOnWarningLog(OnLog&& callback);
        void SetOnErrorLog(OnLog&& callback);

        void SetSceneViewportMousePos(uint32_t x, uint32_t y);
        void SetSelectedEntity(uint32_t entity);

        uint32_t GetPickedEntity() const;

        void UpdateBodyType(BodyHandle handle, PhysicsBodyType type);
        void UpdateBodyScale(BodyHandle handle, const glm::vec3& scale);
        void UpdateBodyMass(BodyHandle handle, float mass);

        bool RequestSimulationState(Scene &scene, SimulationState state);

        bool IsSceneReady(const Scene& scene) const;

        entt::entity SpawnPrimitive(
            Scene &scene,
            std::string_view name,
            MeshType type,
            const Transform &transform
        );
        entt::entity SpawnMesh(
            Scene &scene,
            const std::filesystem::path &path,
            std::string_view name,
            const Transform &transform,
            std::optional<PhysicsCreationOptions> physics
        );

        bool AddPhysicsToEntity(Scene &scene, entt::entity entity, PhysicsCreationOptions options);

        void AddLightEntity(Scene& scene, LightType type);
        void AddCameraEntity(Scene& scene, int width, int height);

        void AddAudioSourceEntity(Scene& scene);
        bool AddAudioSourceToEntity(Scene& scene, entt::entity entity);

        DeviceInfo GetDeviceInfo() const;

        void SetRenderScale(float scale);

        void SetThreadDispatcher(ThreadDispatcher& dispatcher);

        using OnLoadSceneRequest = std::move_only_function<bool(const std::filesystem::path&)>;
        void SetOnLoadSceneRequest(OnLoadSceneRequest&& callback);

        void SetBuiltinCameraControl(bool enabled);
        bool GetBuiltinCameraControl() const;

        friend class ScriptServices;

    private:
        static constexpr std::string_view kDirectoryIconPath = "assets/icons/directory_icon.png";
        static constexpr std::string_view kFileIconPath = "assets/icons/file_icon.png";

        void StartSimulation(Scene& scene);

        void CreateRenderingContext(Window& window);
        void CreateMainPass();
        void CreateSkybox();
        void CreateGizmoPass();
        void CreateOutlinePass();
        void CreateShadowPass();
        void CreateFrameResources();
        void CreateTransferManager();
        void CreateMeshRegistry();
        void CreateTextureRegistry();
        void CreatePhysicsRegistry();
        void CreateAudioRegistry();
        void CreateGizmoRegistry();
        void CreateAssetPipeline();
        void CreatePhysicsSystem();
        void CreateAudioSystem();
        void CreateScriptSystem();
        void CreateImGui(Window& window);

        void SeedDefaultTextures();
        void ResetTextureDescriptors(std::span<const uint32_t> textureSlots);

        void CreateComputePicker();
        void CreateComputeCuller();

        void CreateDirectionalShadowMap();
        void CreatePointShadowMap();

        void CreateSceneViews();
        void AcquireViewTextureIds();

        void CheckRuntimePresentSupport() const;

        bool IsEditor() const;
        bool UsesImGui() const;

        void                                        Submit(const FrameData& frame, vk::Semaphore imageAvailableSemaphore, vk::Semaphore renderFinishedSemaphore) const;
        void                                        RecordMeshData(const FrameData &frame, const CommandRecorder &recorder, uint32_t cameraIndex, bool writeIds) const;
        void                                        RecordSkybox(const FrameData &frame, const CommandRecorder &recorder, uint32_t cameraIndex, bool multisampled) const;
        void                                        RecordDirectionalShadows(const Scene &scene, const FrameData &frame, CommandRecorder &recorder, uint32_t viewIndex) const;
        void                                        RecordPointShadows(const Scene &scene, const FrameData& frame, CommandRecorder &recorder, uint32_t viewIndex) const;
        void                                        RecordGizmos(const Scene &scene, const FrameData &frame, const CommandRecorder &recorder) const;
        void                                        RecordImGuiData(Scene &scene, const FrameData& frame);
        void                                        RecordPicker(const FrameData& frame, const CommandRecorder &recorder) const;
        void                                        RecordOutline(const Scene &scene, const FrameData& frame, const CommandRecorder &recorder);
        void                                        RenderSimulationView(const Scene &scene, const FrameData &frame, CommandRecorder &recorder);
        void                                        RecordEditorPasses(Scene &scene, FrameData& frame, CommandRecorder &recorder, const RenderTarget &sceneView, uint32_t imageIndex);
        void                                        RecordRuntimePresent(Scene &scene, const FrameData& frame, CommandRecorder &recorder, uint32_t imageIndex);
        void                                        RenderScenePass(const FrameData &frame, CommandRecorder &recorder, const RenderTarget &sceneView);
        void                                        RecordImGuiPass(Scene &scene, const FrameData& frame, CommandRecorder &recorder, uint32_t imageIndex);

        static CameraData build_camera_data(const Scene &scene, entt::entity entity, glm::ivec2 extent);

        void UpdateControlledCamera(Scene &scene, float deltaTime);

        void UpdateFrameBuffers(const Scene &scene, FrameData& frame, const RenderTarget &sceneView, const CommandRecorder& recorder);
        void UpdateCameraBuffer(const Scene &scene, FrameData& frame, const RenderTarget &sceneView) const;
        void UpdateMeshBuffers(const Scene &scene, FrameData& frame) const;
        void UpdateMaterialBuffer(FrameData& frame) const;

        void UpdateSceneBuffer(const Scene &scene, FrameData& frame) const;

        void ReadOutputBuffers(const FrameData& frame);

        void RecreateSwapchainResources(const Window& window);

        void ExecuteCulling(
            const Scene &scene,
            const FrameData &frame,
            const CommandRecorder &recorder,
            entt::entity camera,
            vk::Extent2D extent,
            CullingPreset preset
        ) const;

        void ResizeSceneView(glm::ivec2 extent);
        void ResizeSimulationView(glm::ivec2 extent);
        void RetireViews();

        decltype(auto) GetCurrentSimulationView(this auto&& self)
        {
            return (self.mSimulationViews[self.mCurrentFrame]);
        }

        static constexpr float kMinRenderScale{0.25f};

        static glm::ivec2 apply_scale(glm::ivec2 extent, float scale);

        void TransitionForShadowPass(const FrameData& frame, const CommandRecorder& recorder) const;
        void TransitionShadowMapForSampling(const FrameData& frame, const CommandRecorder& recorder) const;

        void TransitionForPointShadowPass(const FrameData& frame, const CommandRecorder& recorder) const;
        void TransitionPointShadowMapForSampling(const FrameData& frame, const CommandRecorder& recorder) const;

        void TransitionForMainPass(const RenderTarget &sceneView, const CommandRecorder& recorder) const;
        void TransitionForSimulationPass(const CommandRecorder &recorder) const;
        void TransitionForGizmoPass(const RenderTarget &sceneView, const CommandRecorder& recorder) const;
        void TransitionForOutlinePass(const RenderTarget &sceneView, const CommandRecorder& recorder, uint32_t imageIndex) const;
        void TransitionForPickerAndPostProcess(const FrameData& frame, const RenderTarget &sceneView, const CommandRecorder& recorder) const;
        void TransitionForPresent(const CommandRecorder& recorder, uint32_t imageIndex) const;
        void TransitionForBlit(const CommandRecorder& recorder, uint32_t imageIndex, bool hasSource) const;
        void TransitionAfterBlit(const CommandRecorder& recorder, uint32_t imageIndex) const;

        EngineSpecification                        mSpecification;
        Context                                    mContext;
        Swapchain                                  mSwapchain;
        ImGuiBackend                               mImGuiBackend;

        TransferManager                            mTransferManager;

        MeshRegistry                               mMeshRegistry;
        TextureRegistry                            mTextureRegistry;
        PhysicsRegistry                            mPhysicsRegistry;
        AudioRegistry                              mAudioRegistry;
        GizmoRegistry                              mGizmoRegistry;

        AssetPipeline                                mAssetPipeline;
        PhysicsSystem                                mPhysicsSystem;
        AudioSystem                                  mAudioSystem;
        ScriptSystem                                 mScriptSystem;
        DeferredResourceEraser<details::kFramesInFlight + 1> mDeferredResourceEraser;

        std::array<FrameData, details::kFramesInFlight>    mFrames;
        uint32_t                                   mCurrentFrame;

        entt::entity                               mControlledCamera{entt::null};
        InputSource                                mInputSource;
        bool                                       mMouseLookActive{};
        bool                                       mSimulationStartPending{};
        bool                                       mBuiltinCameraControl{true};
        bool                                       mStartSimulationOnAttach{};
        glm::vec2                                  mScrollDelta{0.f};
        ThreadDispatcher*                          pThreadDispatcher{};
        OnLoadSceneRequest                         mOnLoadSceneRequest;
        OnImGuiRender                              mOnImGuiRender;

        std::array<RenderTarget, details::kFramesInFlight> mSceneViews;
        std::array<RenderTarget, details::kFramesInFlight> mSimulationViews;
        std::vector<RenderTarget>                          mRetiredViews;
        ResizeDebouncer<>                                  mSceneResize;
        ResizeDebouncer<>                                  mSimulationResize;
        bool                                               mSimulationViewActive{};

        glm::ivec2 mSceneViewExtent{};
        float      mRenderScale{1.f};

        static constexpr auto   kSceneSamples{vk::SampleCountFlagBits::e1};
        vk::SampleCountFlagBits mSimulationSamples{vk::SampleCountFlagBits::e1};

        compute_pass_push_constants::MouseCords     mSceneViewportMousePos;
        graphics_pass_push_constants::Outline       mOutlineInfo;

        MainPass                                   mMainPass;
        SkyboxPass                                 mSkyboxPass;
        GizmoPass                                  mGizmoPass;
        OutlinePass                                mOutlinePass;
        ComputePicker                              mComputePicker;
        uint32_t                                   mPickedEntity;
        ComputeCuller                              mComputeCuller;
        ShadowPass                                 mShadowPass;

        ShadowMap                                  mDirectionalShadowMap;
        ShadowMap                                  mPointShadowMap;

        std::array<DirectionalShadowSet, details::kMaxCameraViews> mDirectionalShadowSets;
        std::array<PointShadowSet, details::kMaxCameraViews>       mPointShadowSets;

        OnLog                                      mOnInfoLog;
        OnLog                                      mOnWarningLog;
        OnLog                                      mOnErrorLog;
    };
}
