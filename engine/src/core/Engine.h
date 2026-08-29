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
        void setOnEditorRender(OnEditorRender&& callback);

        CameraData getCameraData() const;

        void setSimulationViewExtent(vk::Extent2D extent);
        void setSimulationViewActive(bool active);
        void setControlledCamera(entt::entity camera);

        void waitIdle() const;

        Queue<AssetPipeline::PendingMeshData> &getPendingMeshDataQueue();

        void unregisterMesh(MeshHandle handle, std::string_view path);
        void unregisterMaterial(MaterialHandle handle);

        ImTextureID getAssetBrowserDirectoryTextureId() const;
        ImTextureID getAssetBrowserFileTextureId() const;
        ImTextureID getSceneTextureId() const;
        ImTextureID getSimulationTextureId() const;

        void onEvent(const Event& event, Window& window);
        void update(float deltaTime, const Window &window);
        void render(const Window &window);

        static bool is_mesh_type_supported(std::string_view path);
        static bool is_image_type_supported(std::string_view path);

        bool isMeshCached(std::string_view path) const;

        static constexpr std::string_view kSceneFileExtension = "klx";
        const Scene& getScene() const;
        void         saveScene(const std::filesystem::path &path);
        void         loadScene(const std::filesystem::path &path, const Window &window);

        using OnLog = std::move_only_function<void(std::string_view)>;
        void setOnInfoLog(OnLog&& callback);
        void setOnWarningLog(OnLog&& callback);
        void setOnErrorLog(OnLog&& callback);

        void setSceneViewportMousePos(uint32_t x, uint32_t y);
        void setOutlineInfo(glm::vec3 color, uint32_t entity);

        uint32_t getPickedEntity() const;

        void updateBodyType(BodyHandle handle, PhysicsBodyType type);
        void updateBodyScale(BodyHandle handle, const glm::vec3& scale);
        void setSimulationState(SimulationState state);

        void addPhysicsToEntity(entt::entity entity, PhysicsCreationOptions options);

        void addLightEntity(LightType type);

        DeviceInfo getDeviceInfo() const;

    private:
        static constexpr std::string_view kDirectoryIconPath = "assets/icons/directory_icon.png";
        static constexpr std::string_view kFileIconPath = "assets/icons/file_icon.png";

        void createRenderingContext(Window& window);
        void createMainPass();
        void createSkybox();
        void createGizmoPass();
        void createOutlinePass();
        void createFrameResources();
        void createTransferManager();
        void createMeshRegistry();
        void createTextureRegistry();
        void createPhysicsRegistry();
        void createGizmoRegistry();
        void createAssetPipeline();
        void createPhysicsSystem();
        void createImGui(Window& window);

        void seedDefaultTextures();

        void createEditorTextureIds();

        void createComputePicker();
        void createComputeCuller();

        void createScene(const Window &window);

        void                                        submit(const FrameData& frame, vk::Semaphore imageAvailableSemaphore, vk::Semaphore renderFinishedSemaphore) const;
        void                                        recordMeshData(const FrameData &frame, const CommandRecorder &recorder, uint32_t cameraIndex, bool writeIds) const;
        void                                        recordSkybox(const FrameData &frame, const CommandRecorder &recorder, uint32_t cameraIndex) const;
        void                                        recordGizmos(const FrameData &frame, const CommandRecorder &recorder) const;
        void                                        recordImGuiData(const FrameData& frame);
        void                                        recordPicker(const FrameData& frame, const CommandRecorder &recorder) const;
        void                                        recordOutline(const FrameData& frame, const CommandRecorder &recorder) const;
        void                                        renderSimulationView(const FrameData &frame, CommandRecorder &recorder);

        CameraData buildCameraData(entt::entity entity, vk::Extent2D extent) const;

        void updateFrameBuffers(FrameData& frame, const CommandRecorder& recorder);
        void updateCameraBuffer(FrameData& frame) const;
        void updateMeshDataBuffer(FrameData& frame) const;
        void updateMaterialBuffer(FrameData& frame) const;

        void updateSceneBuffer(FrameData& frame) const;
        void updateCullerBuffers(const FrameData& frame, const CommandRecorder &recorder);

        void readOutputBuffers(const FrameData& frame);

        BodyHandle uploadPhysicsBodyDataToRegistry(const PhysicsBodyInfo& data);

        void executeCulling(const FrameData& frame, const CommandRecorder& recorder, entt::entity camera, vk::Extent2D extent);

        void resizeSimulationView();

        static void copy_scene_to_simulation_texture(const FrameData& frame, const CommandRecorder& recorder);
        static bool needs_resize(vk::Extent2D extentA, vk::Extent2D extentB);

        void transitionForMainPass(const FrameData& frame, const CommandRecorder& recorder) const;
        void transitionForSimulationPass(const CommandRecorder &recorder) const;
        void transitionForGizmoPass(const FrameData& frame, const CommandRecorder& recorder) const;
        void transitionForOutlinePass(const FrameData& frame, const CommandRecorder& recorder, uint32_t imageIndex) const;
        void transitionForPickerAndPostProcess(const FrameData& frame, const CommandRecorder& recorder) const;
        void transitionForPresent(const CommandRecorder& recorder, uint32_t imageIndex) const;

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
        bool                                       mMouseLookActive{};
        OnEditorRender                             mOnEditorRender;

        SimulationView                             mSimulationView;
        vk::Extent2D                               mRequestedSimulationExtent{};
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
