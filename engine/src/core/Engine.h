#pragma once

#include "Clock.h"
#include "Context.h"
#include "descriptor/DescriptorLayout.h"
#include "Swapchain.h"
#include "FrameData.h"
#include "Pipeline.h"
#include "imgui_backend/ImGuiBackend.h"
#include "window/Event.h"

#include "descriptor/DescriptorPool.h"
#include "mesh/MeshRegistry.h"
#include <entt/entt.hpp>

#include "AssetPipeline.h"
#include "passes/ComputePicker.h"
#include "scene/Scene.h"
#include "TransferManager.h"
#include "mesh/MeshLoader.h"
#include "passes/MainPass.h"
#include "physics/PhysicsRegistry.h"
#include "physics/PhysicsSystem.h"
#include "utilities/Queue.h"
#include "utilities/ThreadDispatcher.h"
#include "DeferredResourceEraser.h"
#include "ResizeDebouncer.h"
#include "SimulationView.h"
#include "components/gpu/CameraData.h"
#include "gizmo/GizmoRegistry.h"
#include "passes/GizmoPass.h"

namespace kailux
{
    class Engine
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Engine)
        ~Engine();

        static Engine create(Window& window);

        using OnEditorRender = std::move_only_function<void(Scene&)>;
        void SetOnEditorRender(OnEditorRender&& callback);

        CameraData GetCameraData() const;

        void SetSimulationViewExtent(vk::Extent2D extent);
        void SetSimulationViewActive(bool active);
        void SetControlledCamera(entt::entity camera, InputSource source);
        void ToggleMouseLook();

        void WaitIdle() const;

        Queue<AssetPipeline::PendingMeshData> &GetPendingMeshDataQueue();

        void UnregisterMesh(MeshHandle handle, std::string_view path);
        void UnregisterMaterial(MaterialHandle handle);

        ImTextureID GetAssetBrowserDirectoryTextureId() const;
        ImTextureID GetAssetBrowserFileTextureId() const;
        ImTextureID GetSceneTextureId() const;
        ImTextureID GetSimulationTextureId() const;

        void OnEvent(const Event& event, Window& window);
        void Update(float deltaTime);
        void Render(const Window &window);

        static bool is_mesh_type_supported(std::string_view path);
        static bool is_image_type_supported(std::string_view path);

        bool IsMeshCached(std::string_view path) const;

        static constexpr std::string_view kSceneFileExtension = "klx";
        const Scene& GetScene() const;
        void         SaveScene(const std::filesystem::path &path);
        void         LoadScene(const std::filesystem::path &path, const Window &window);

        using OnLog = std::move_only_function<void(std::string_view)>;
        void SetOnInfoLog(OnLog&& callback);
        void SetOnWarningLog(OnLog&& callback);
        void SetOnErrorLog(OnLog&& callback);

        void SetSceneViewportMousePos(uint32_t x, uint32_t y);
        void SetOutlineInfo(glm::vec3 color, uint32_t entity);

        uint32_t GetPickedEntity() const;

        void UpdateBodyType(BodyHandle handle, PhysicsBodyType type);
        void UpdateBodyScale(BodyHandle handle, const glm::vec3& scale);
        void SetSimulationState(SimulationState state);

        void AddPhysicsToEntity(entt::entity entity, PhysicsCreationOptions options);

        void AddLightEntity(LightType type);

        DeviceInfo GetDeviceInfo() const;

    private:
        static constexpr std::string_view kDirectoryIconPath = "assets/icons/directory_icon.png";
        static constexpr std::string_view kFileIconPath = "assets/icons/file_icon.png";

        void CreateRenderingContext(Window& window);
        void CreateMainPass();
        void CreateSkybox();
        void CreateGizmoPass();
        void CreateOutlinePass();
        void CreateFrameResources();
        void CreateTransferManager();
        void CreateMeshRegistry();
        void CreateTextureRegistry();
        void CreatePhysicsRegistry();
        void CreateGizmoRegistry();
        void CreateAssetPipeline();
        void CreatePhysicsSystem();
        void CreateImGui(Window& window);

        void SeedDefaultTextures();

        void CreateEditorTextureIds();

        void CreateComputePicker();
        void CreateComputeCuller();

        void CreateScene(const Window &window);

        void                                        Submit(const FrameData& frame, vk::Semaphore imageAvailableSemaphore, vk::Semaphore renderFinishedSemaphore) const;
        void                                        RecordMeshData(const FrameData &frame, const CommandRecorder &recorder, uint32_t cameraIndex, bool writeIds) const;
        void                                        RecordSkybox(const FrameData &frame, const CommandRecorder &recorder, uint32_t cameraIndex) const;
        void                                        RecordGizmos(const FrameData &frame, const CommandRecorder &recorder) const;
        void                                        RecordImGuiData(const FrameData& frame);
        void                                        RecordPicker(const FrameData& frame, const CommandRecorder &recorder) const;
        void                                        RecordOutline(const FrameData& frame, const CommandRecorder &recorder) const;
        void                                        RenderSimulationView(const FrameData &frame, CommandRecorder &recorder);

        CameraData BuildCameraData(entt::entity entity, vk::Extent2D extent) const;

        void UpdateFrameBuffers(FrameData& frame, const CommandRecorder& recorder);
        void UpdateCameraBuffer(FrameData& frame) const;
        void UpdateMeshDataBuffer(FrameData& frame) const;
        void UpdateMaterialBuffer(FrameData& frame) const;

        void UpdateSceneBuffer(FrameData& frame) const;
        void UpdateCullerBuffers(const FrameData& frame, const CommandRecorder &recorder);

        void ReadOutputBuffers(const FrameData& frame);

        void RecreateSwapchainResources(const Window& window);

        BodyHandle UploadPhysicsBodyDataToRegistry(const PhysicsBodyInfo& data);

        void ExecuteCulling(const FrameData& frame, const CommandRecorder& recorder, entt::entity camera, vk::Extent2D extent);

        void ResizeSimulationView(vk::Extent2D extent);

        void TransitionForMainPass(const FrameData& frame, const CommandRecorder& recorder) const;
        void TransitionForSimulationPass(const CommandRecorder &recorder) const;
        void TransitionForGizmoPass(const FrameData& frame, const CommandRecorder& recorder) const;
        void TransitionForOutlinePass(const FrameData& frame, const CommandRecorder& recorder, uint32_t imageIndex) const;
        void TransitionForPickerAndPostProcess(const FrameData& frame, const CommandRecorder& recorder) const;
        void TransitionForPresent(const CommandRecorder& recorder, uint32_t imageIndex) const;

        Context                                    mContext;
        vk::SampleCountFlagBits                    mSampleCount;
        Swapchain                                  mSwapchain;
        ImGuiBackend                               mImGuiBackend;

        TransferManager                            mTransferManager;

        MeshRegistry                               mMeshRegistry;
        TextureRegistry                            mTextureRegistry;
        PhysicsRegistry                            mPhysicsRegistry;
        GizmoRegistry                              mGizmoRegistry;

        AssetPipeline                                mAssetPipeline;
        PhysicsSystem                                mPhysicsSystem;
        DeferredResourceEraser<details::kFramesInFlight + 1> mDeferredResourceEraser;

        std::array<FrameData, details::kFramesInFlight>    mFrames;
        uint32_t                                   mCurrentFrame;

        std::array<ImTextureID, details::kFramesInFlight>  mSceneTextureIds{};
        std::array<ImTextureID, details::kFramesInFlight>  mSimulationTextureIds{};

        Scene                                      mScene;
        entt::entity                               mControlledCamera{entt::null};
        InputSource                                mInputSource;
        bool                                       mMouseLookActive{};
        OnEditorRender                             mOnEditorRender;

        SimulationView                             mSimulationView;
        SimulationView                             mRetiredSimulationView;
        ResizeDebouncer<>                          mSimulationResize;
        bool                                       mSimulationViewActive{};

        ComputePassesPushConstants::MouseCords     mSceneViewportMousePos;
        GraphicsPassesPushConstants::Outline       mOutlineInfo;

        MainPass                                   mMainPass;
        SkyboxPass                                 mSkyboxPass;
        GizmoPass                                  mGizmoPass;
        OutlinePass                                mOutlinePass;
        ComputePicker                              mComputePicker;
        uint32_t                                   mPickedEntity;
        ComputeCuller                              mComputeCuller;

        OnLog                                      mOnInfoLog;
        OnLog                                      mOnWarningLog;
        OnLog                                      mOnErrorLog;
    };
}
