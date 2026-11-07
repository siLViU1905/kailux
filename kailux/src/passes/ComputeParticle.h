#pragma once

#include "ComputePass.h"
#include "ComputePassesPushConstants.h"

namespace kailux 
{
    class ComputeParticle : public ComputePass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ComputeParticle)

        static ComputeParticle create(const Context& context, uint32_t frameCount);

        template<typename... Pcs>
        void Push(vk::CommandBuffer cmd, const Pcs &... pcs) const
        {
            PushImpl<kPushConstantRanges, Pcs...>(cmd, pcs...);
        }

        static constexpr uint32_t kWorkgroupSize{256};

    private:
        static constexpr std::string_view kComputeShaderPath = "shaders/particle_compute_shader.glsl";

        static constexpr std::array kDescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // particles
                vk::ShaderStageFlagBits::eCompute
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // emitters
                vk::ShaderStageFlagBits::eCompute
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // bursts
                vk::ShaderStageFlagBits::eCompute
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // spawn counters
                vk::ShaderStageFlagBits::eCompute
            )
        };
        static constexpr std::array kDescriptorPoolSizes = {
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // particles
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // emitters
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // bursts
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // spawn counters
            )
        };
        static_assert(
            check_descriptor_layout_bindings_and_pool_sizes_match(kDescriptorLayoutBindings, kDescriptorPoolSizes),
            "Descriptor layout bindings and pool sizes do not match"
            );

        static constexpr std::array kPushConstantRanges = {
            PushConstantRangeInfo(
                vk::ShaderStageFlagBits::eCompute,
                sizeof(compute_pass_push_constants::ParticleSimulation)
            )
        };
    };
}