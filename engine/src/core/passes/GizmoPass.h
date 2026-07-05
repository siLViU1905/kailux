#pragma once
#include "GraphicsPass.h"
#include "GraphicsPassesPushConstants.h"

namespace kailux
{
    class GizmoPass : public GraphicsPass
    {
        public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(GizmoPass)

        static GizmoPass create(const Context& context, const Swapchain& swapchain, uint32_t maxFrames);

        template<typename... Pcs>
        void push(vk::CommandBuffer cmd, const Pcs &... pcs) const
        {
            pushImpl<kPushConstantRanges, Pcs...>(cmd, pcs...);
        }

    private:
        static constexpr std::string_view kVertexShaderPath = "shaders/gizmo_vertex_shader.spv";
        static constexpr std::string_view kFragmentShaderPath = "shaders/gizmo_fragment_shader.spv";

        static constexpr std::array kDescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eUniformBuffer,
                1, // camera
                vk::ShaderStageFlagBits::eVertex
            )
        };
        static constexpr std::array kDescriptorPoolSizes = {
            DescriptorPoolSize(
                vk::DescriptorType::eUniformBuffer,
                1 // camera
            )
        };
        static_assert(
            check_descriptor_layout_bindings_and_pool_sizes_match(kDescriptorLayoutBindings, kDescriptorPoolSizes),
            "Descriptor layout bindings and pool sizes do not match"
            );

        static constexpr std::array kPushConstantRanges = {
            PushConstantRangeInfo(vk::ShaderStageFlagBits::eVertex,
                sizeof(GraphicsPassesPushConstants::Gizmo)
                )
        };

        static PipelineInfo make_pipeline_info(const Swapchain& swapchain, vk::SampleCountFlagBits sampleCount);
    };
}
