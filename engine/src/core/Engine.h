#pragma once
#include "Context.h"
#include "DescriptorSetLayout.h"
#include "Swapchain.h"
#include "FrameData.h"
#include "Pipeline.h"
#include "imgui_backend/ImGuiBackend.h"
#include "window/Event.h"
#include <functional>

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

        static PipelineInfo make_pipeline_info(vk::SampleCountFlagBits sampleCount);

        void submit(const FrameData& frame, vk::Semaphore imageAvailableSemaphore, vk::Semaphore renderFinishedSemaphore) const;
        void render(Window &window);
        void recordMeshData(vk::CommandBuffer cmd) const;
        void recordImGuiData(const FrameData& frame);

        void handleEvent(Event event);

        static constexpr uint32_t s_FramesInFlight = 2;

        Context                                 m_Context;
        vk::SampleCountFlagBits                 m_SampleCount;
        Swapchain                               m_Swapchain;
        ImGuiBackend                            m_ImGuiBackend;
        DescriptorSetLayout                     m_DescriptorSetLayout;
        Pipeline                                m_Pipeline;
        MeshRegistry                            m_MeshRegistry;
        std::array<FrameData, s_FramesInFlight> m_Frames;
        uint32_t                                m_CurrentFrame;
    };
}
