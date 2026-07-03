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
#include "Scene.h"
#include "TransferManager.h"
#include "mesh/MeshLoader.h"
#include "passes/MainPass.h"
#include "physics/PhysicsRegistry.h"
#include "physics/PhysicsSystem.h"
#include "utilities/Queue.h"
#include "utilities/ThreadDispatcher.h"
#include "DeferredResourceEraser.h"

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

        void waitIdle() const;

        struct PendingMeshData
        {
            std::string          path;
            MeshLoader::LoadData data;
            std::string          name;
            MeshTransformData    transform;
            MeshMaterialData     material;
            MeshType             type{MeshType::Unknown};
            PhysicsBodyType      bodyType{PhysicsBodyType::Static};
        };

        Queue<AssetPipeline::PendingMeshData> &getPendingMeshDataQueue();

        void unregisterMesh(MeshHandle handle, std::string_view path);
        void unregisterTextureSet(TextureSetHandle handle);

        ImTextureID getAssetBrowserDirectoryTextureId() const;
        ImTextureID getAssetBrowserFileTextureId() const;
        ImTextureID getSceneTextureId() const;

        void update(float deltaTime, Window &window);
        void render(const Window &window);

        static bool is_mesh_type_supported(std::string_view path);
        static bool is_image_type_supported(std::string_view path);

        bool isMeshCached(std::string_view path) const;

        static constexpr std::string_view kSceneFileExtension = "klx";
        void saveScene(std::string_view folder) const;
        void loadScene(std::string_view path, int windowWidth, int windowHeight);

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

    private:
        static constexpr uint32_t         kMaxMeshCount = MainPass::kMaxMeshCount;

        static constexpr std::string_view kDirectoryIconPath = "assets/icons/directory_icon.png";
        static constexpr std::string_view kFileIconPath = "assets/icons/file_icon.png";

        void createRenderingContext(Window& window);
        void createMainPass();
        void createSkybox();
        void createOutlinePass();
        void createFrameResources();
        void createTransferManager();
        void createMeshRegistry();
        void createTextureRegistry();
        void createPhysicsRegistry();
        void createAssetPipeline();
        void createPhysicsSystem();
        void createImGui(Window& window);

        void createSceneTextureIds();

        void createComputePicker();
        void createComputeCuller();

        void createScene();
        void createSceneEntities(const Window &window);

        static std::array<DescriptorSetUpdateInfo, TextureRegistry::kTextureTypes.size()> make_descriptor_set_update_info_from_texture_set(TextureSetHandle slotToOverwrite, const TextureSet& replacementSet);

        void                                        submit(const FrameData& frame, vk::Semaphore imageAvailableSemaphore, vk::Semaphore renderFinishedSemaphore) const;
        void                                        recordMeshData(const FrameData &frame, const CommandRecorder &recorder) const;
        void                                        recordSkybox(const FrameData &frame, const CommandRecorder &recorder) const;
        void                                        recordImGuiData(const FrameData& frame);
        void                                        recordPicker(const FrameData& frame, const CommandRecorder &recorder) const;
        void                                        recordOutline(const FrameData& frame, const CommandRecorder &recorder) const;

        void updateFrameBuffers(FrameData& frame, const CommandRecorder& recorder);
        void updateCameraBuffer(FrameData& frame) const;
        void updateMeshDataBuffer(FrameData& frame) const;

        void updateSceneBuffer(FrameData& frame) const;
        void updateCullerBuffers(const FrameData& frame, const CommandRecorder &recorder);

        void readOutputBuffers(const FrameData& frame);

        void handleEvent(Window &window);

        BodyHandle uploadPhysicsBodyDataToRegistry(const PhysicsBodyInfo& data);

        void executeCulling(const FrameData& frame, const CommandRecorder& recorder);

        void transitionForMainPass(const FrameData& frame, const CommandRecorder& recorder) const;
        void transitionForOutlinePass(const FrameData& frame, const CommandRecorder& recorder, uint32_t imageIndex) const;
        void transitionForPickerAndPostProcess(const FrameData& frame, const CommandRecorder& recorder) const;
        void transitionForPresent(const CommandRecorder& recorder, uint32_t imageIndex) const;

        static constexpr uint32_t kFramesInFlight = 2;

        Context                                    m_Context;
        vk::SampleCountFlagBits                    m_SampleCount;
        Swapchain                                  m_Swapchain;
        ImGuiBackend                               m_ImGuiBackend;

        TransferManager                            m_TransferManager;

        MeshRegistry                               m_MeshRegistry;
        TextureRegistry                            m_TextureRegistry;
        PhysicsRegistry                            m_PhysicsRegistry;

        AssetPipeline                                m_AssetPipeline;
        PhysicsSystem                                m_PhysicsSystem;
        DeferredResourceEraser<kFramesInFlight + 1> m_DeferredResourceEraser;

        std::array<FrameData, kFramesInFlight>    m_Frames;
        uint32_t                                   m_CurrentFrame;

        std::array<ImTextureID, kFramesInFlight>  m_SceneTextureIds;

        Scene                                      m_Scene;
        OnEditorRender                             m_OnEditorRender;

        ComputePassesPushConstants::MouseCords     m_SceneViewportMousePos;
        GraphicsPassesPushConstants::Outline       m_OutlineInfo;

        MainPass                                   m_MainPass;
        SkyboxPass                                 m_SkyboxPass;
        OutlinePass                                m_OutlinePass;
        ComputePicker                              m_ComputePicker;
        uint32_t                                   m_PickedEntity;
        ComputeCuller                              m_ComputeCuller;

        OnLog                                      m_OnInfoLog;
        OnLog                                      m_OnWarningLog;
        OnLog                                      m_OnErrorLog;
    };
}
