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
        void createCamera(Window& window);

        //only one binding for now, for the camera uniform buffer
        static constexpr std::array<DescriptorLayoutBinding, 1> make_descriptor_layout_bindings(uint32_t uniformBufferCount)
        {
            return {
                DescriptorLayoutBinding(
                    vk::DescriptorType::eUniformBuffer,
                    uniformBufferCount,
                    vk::ShaderStageFlagBits::eVertex
                )
            };
        }
        static constexpr std::array<DescriptorPoolSize, 1> make_descriptor_pool_sizes(uint32_t uniformBufferCount)
        {
            return {
                DescriptorPoolSize(
                    vk::DescriptorType::eUniformBuffer,
                    uniformBufferCount
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
        void                                        recordMeshData(const FrameData &frame, const CommandRecorder &recorder, uint32_t meshCount) const;
        void                                        recordImGuiData(const FrameData& frame);

        void updateFrameBuffers(FrameData& frame, const CommandRecorder& recorder) const;

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
        Camera                                  m_Camera;
    };
}
