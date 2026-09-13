#pragma once
#include "GraphicsPass.h"
#include "GraphicsPassesPushConstants.h"

namespace kailux
{
    class ShadowPass : public GraphicsPass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ShadowPass)

        static ShadowPass create(const Context &context, const Swapchain &swapchain, uint32_t maxFrames);

        template<typename... Pcs>
        void Push(vk::CommandBuffer cmd, const Pcs &... pcs) const
        {
            PushImpl<kPushConstantRanges, Pcs...>(cmd, pcs...);
        }

    private:
        static constexpr std::string_view kVertexShaderPath = "shaders/shadow_vertex_shader.glsl";

        static constexpr std::array kDescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // model
                vk::ShaderStageFlagBits::eVertex
            )
        };
        static constexpr std::array kDescriptorPoolSizes = {
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // model
            )
        };
        static_assert(
            check_descriptor_layout_bindings_and_pool_sizes_match(kDescriptorLayoutBindings, kDescriptorPoolSizes),
            "Descriptor layout bindings and pool sizes do not match"
        );

        static constexpr std::array kPushConstantRanges = {
            PushConstantRangeInfo(vk::ShaderStageFlagBits::eVertex,
                                  sizeof(GraphicsPassesPushConstants::ShadowCascade)
            )
        };

        static PipelineInfo make_pipeline_info();
    };
}
