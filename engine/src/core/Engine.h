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

#include "Scene.h"
#include "mesh/MeshLoader.h"
#include "utilities/Queue.h"
#include "utilities/ThreadDispatcher.h"

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

        Queue<MeshLoader::LoadData> &getPendingDataQueue();

        void unregisterMesh(MeshHandle handle);
        void unregisterTextureSet(TextureSetHandle handle);

        ImTextureID getAssetBrowserDirectoryTextureId() const;
        ImTextureID getAssetBrowserFileTextureId() const;

        void update(float deltaTime, Window &window);
        void render(Window &window);

        static bool is_mesh_type_supported(std::string_view path);
        static bool is_image_type_supported(std::string_view path);

    private:
        static constexpr std::string_view s_VertexShaderPath = "shaders/vertex_shader.spv";
        static constexpr std::string_view s_FragmentShaderPath = "shaders/fragment_shader.spv";
        static constexpr uint32_t         s_MaxMeshCount = 1'000;
        static constexpr std::array<std::string_view, 6> s_SkyboxTexturePaths = {
            "assets/cubemap/px.png",
            "assets/cubemap/nx.png",
            "assets/cubemap/py.png",
            "assets/cubemap/ny.png",
            "assets/cubemap/pz.png",
            "assets/cubemap/nz.png"
        };
        static constexpr std::string_view s_DirectoryIconPath = "assets/icons/directory_icon.png";
        static constexpr std::string_view s_FileIconPath = "assets/icons/file_icon.png";

        void createRenderingContext(Window& window);
        void createDescriptorResources();
        void createPipeline();
        void createSkybox();
        void createFrameResources();
        void createMeshRegistry();
        void createTextureRegistry();
        void createImGui(Window& window);
        void createScene();
        void createSceneEntities(const Window &window);

        static constexpr uint32_t   s_MeshTextureBindStart = 6;
        static constexpr std::array s_DescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eUniformBuffer,
                1, // camera
                vk::ShaderStageFlagBits::eVertex
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // model
                vk::ShaderStageFlagBits::eVertex
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // scene
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                1, // skybox
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                1, // irradiance map
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                1, // brdf lut
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount, // albedo
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount, // normal
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount, // roughness
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount, // metallic
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount, // ao
                vk::ShaderStageFlagBits::eFragment
            )
        };
        static constexpr std::array s_DescriptorPoolSizes = {
            DescriptorPoolSize(
                vk::DescriptorType::eUniformBuffer,
                1 // camera
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // model
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // scene
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                1 // skybox
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                1 // irradiance map
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                1 // brdf lut
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount // albedo
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount // normal
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount // roughness
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount // metallic
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount // ao
            )
        };
        static constexpr bool check_descriptor_layout_bindings_and_pool_sizes_match(std::span<const DescriptorLayoutBinding> bindings, std::span<const DescriptorPoolSize> sizes)
        {
            if (bindings.size() != sizes.size())
                return false;

            for (size_t i = 0; i < bindings.size(); i++)
                if (bindings[i].type != sizes[i].type ||
                    bindings[i].count != sizes[i].count)
                    return false;

            return true;
        }
        static PipelineInfo                                                                make_pipeline_info(vk::SampleCountFlagBits sampleCount);
        static std::array<DescriptorSetUpdateInfo, TextureRegistry::s_TextureTypes.size()> make_descriptor_set_update_info_from_texture_set(TextureSetHandle handle, const TextureSet& set);

        void                                        submit(const FrameData& frame, vk::Semaphore imageAvailableSemaphore, vk::Semaphore renderFinishedSemaphore) const;
        std::vector<vk::DrawIndexedIndirectCommand> getMeshIndirectCommands() const;
        void                                        recordMeshData(const FrameData &frame, const CommandRecorder &recorder) const;
        void                                        recordImGuiData(const FrameData& frame);

        void updateFrameBuffers(FrameData& frame, const CommandRecorder& recorder) const;
        void updateCameraBuffer(FrameData& frame) const;
        void updateMeshDataBuffer(FrameData& frame) const;
        void updateIndirectBuffer(FrameData& frame) const;
        void updateSceneBuffer(FrameData& frame) const;

        void handleEvent(Window &window);

        void             pollPendingData();
        MeshHandle       uploadMeshDataToRegistry(const MeshRegistry::MeshData& data);
        TextureSetHandle uploadMaterialDataToRegistry(const TextureRegistry::MaterialData &data);

        void updatePendingFrameTasks();

        static constexpr uint32_t s_FramesInFlight = 2;

        Context                                 m_Context;
        vk::SampleCountFlagBits                 m_SampleCount;
        Swapchain                               m_Swapchain;
        ImGuiBackend                            m_ImGuiBackend;
        DescriptorLayout                        m_DescriptorLayout;
        DescriptorPool                          m_DescriptorPool;
        Pipeline                                m_Pipeline;
        MeshRegistry                            m_MeshRegistry;
        TextureRegistry                         m_TextureRegistry;
        std::array<FrameData, s_FramesInFlight> m_Frames;
        uint32_t                                m_CurrentFrame;

        Scene                                   m_Scene;
        OnEditorRender                          m_OnEditorRender;
        SkyboxPass                              m_Skybox;

        Queue<MeshLoader::LoadData>             m_PendingData;

        static constexpr uint32_t               s_ResourceCleanupFrameDelay = s_FramesInFlight + 1;
        using FrameTask = std::move_only_function<void()>;
        struct PendingFrameTask
        {
            FrameTask task;
            uint32_t  remainingFrames = s_ResourceCleanupFrameDelay;
        };
        std::vector<PendingFrameTask>           m_PendingFrameTasks;
    };
}
