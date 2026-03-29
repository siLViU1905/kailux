#pragma once
#include "Camera.h"
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

namespace kailux
{
    class Engine
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Engine)
        ~Engine();

        static Engine create(Window& window);

        void run(Window& window);

    private:
        static constexpr std::string_view s_VertexShaderPath = "shaders/vertex_shader.spv";
        static constexpr std::string_view s_FragmentShaderPath = "shaders/fragment_shader.spv";
        static constexpr uint32_t         s_MaxMeshCount = 1'000;

        void createRenderingContext(Window& window);
        void createDescriptorResources();
        void createPipeline();
        void createFrameResources();
        void createMeshRegistry();
        void createImGui(Window& window);
        void createEntities(const Window &window);

        static constexpr uint32_t s_DescriptorLayoutBindingsCount = 1 + 1; // camera buffer + model buffer
        static constexpr std::array<DescriptorLayoutBinding, s_DescriptorLayoutBindingsCount> make_descriptor_layout_bindings(uint32_t uniformBufferCount, uint32_t storageBufferCount)
        {
            return {
                DescriptorLayoutBinding(
                    vk::DescriptorType::eUniformBuffer,
                    uniformBufferCount,
                    vk::ShaderStageFlagBits::eVertex
                ),
                DescriptorLayoutBinding(
                    vk::DescriptorType::eStorageBuffer,
                    storageBufferCount,
                    vk::ShaderStageFlagBits::eVertex
                )
            };
        }
        static constexpr uint32_t s_DescriptorPoolSizesCount = s_DescriptorLayoutBindingsCount;
        static constexpr std::array<DescriptorPoolSize, s_DescriptorPoolSizesCount> make_descriptor_pool_sizes(uint32_t uniformBufferCount, uint32_t storageBufferCount)
        {
            return {
                DescriptorPoolSize(
                    vk::DescriptorType::eUniformBuffer,
                    uniformBufferCount
                ),
                DescriptorPoolSize(
                   vk::DescriptorType::eStorageBuffer,
                   storageBufferCount
               )
            };
        }
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
        static PipelineInfo make_pipeline_info(vk::SampleCountFlagBits sampleCount);

        void                                        submit(const FrameData& frame, vk::Semaphore imageAvailableSemaphore, vk::Semaphore renderFinishedSemaphore) const;
        void                                        render(Window &window);
        std::vector<vk::DrawIndexedIndirectCommand> getMeshIndirectCommands() const;
        void                                        recordMeshData(const FrameData &frame, const CommandRecorder &recorder) const;
        void                                        recordImGuiData(const FrameData& frame);


        void updateFrameBuffers(FrameData& frame, const CommandRecorder& recorder) const;
        void updateCameraBuffer(FrameData& frame) const;
        void updateModelBuffer(FrameData& frame) const;
        void updateIndirectBuffer(FrameData& frame) const;

        void handleEvent(Window &window);

        static constexpr uint32_t s_FramesInFlight = 2;

        Context                                 m_Context;
        vk::SampleCountFlagBits                 m_SampleCount;
        Swapchain                               m_Swapchain;
        ImGuiBackend                            m_ImGuiBackend;
        DescriptorLayout                        m_DescriptorLayout;
        DescriptorPool                          m_DescriptorPool;
        Pipeline                                m_Pipeline;
        MeshRegistry                            m_MeshRegistry;
        std::array<FrameData, s_FramesInFlight> m_Frames;
        uint32_t                                m_CurrentFrame;

        Clock                                   m_Clock;

        entt::registry                          m_EntityRegistry;
        entt::entity                            m_MainCameraEntity;
    };
}
