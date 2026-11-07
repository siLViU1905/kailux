#pragma once

#include "GraphicsPass.h"
#include "GraphicsPassesPushConstants.h"

namespace kailux
{
    class ParticlePass : public GraphicsPass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ParticlePass)

        static ParticlePass create(const Context &context, const Swapchain &swapchain, uint32_t maxFrames);

        template<typename... Pcs>
        void Push(vk::CommandBuffer cmd, const Pcs &... pcs) const
        {
            PushImpl<kPushConstantRanges, Pcs...>(cmd, pcs...);
        }

        void Bind(vk::CommandBuffer cmd, bool multisampled) const;

    private:
        static constexpr std::string_view kVertexShaderPath = "shaders/particle_vertex_shader.glsl";
        static constexpr std::string_view kFragmentShaderPath = "shaders/particle_fragment_shader.glsl";

        static constexpr std::array kDescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eUniformBuffer,
                1, // camera
                vk::ShaderStageFlagBits::eVertex
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // particles
                vk::ShaderStageFlagBits::eVertex
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // emitters
                vk::ShaderStageFlagBits::eVertex
            )
        };
        static constexpr std::array kDescriptorPoolSizes = {
            DescriptorPoolSize(
                vk::DescriptorType::eUniformBuffer,
                1 // camera
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // particles
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // emitters
            )
        };
        static_assert(
            check_descriptor_layout_bindings_and_pool_sizes_match(kDescriptorLayoutBindings, kDescriptorPoolSizes),
            "Descriptor layout bindings and pool sizes do not match"
            );

        static constexpr std::array kPushConstantRanges = {
            PushConstantRangeInfo(vk::ShaderStageFlagBits::eVertex,
                sizeof(graphics_pass_push_constants::Particles)
                )
        };

        static PipelineInfo make_pipeline_info(const Swapchain &swapchain, vk::SampleCountFlagBits samples, bool writeIds);

        GraphicsPipeline mMultisampledPipeline;
    };
}