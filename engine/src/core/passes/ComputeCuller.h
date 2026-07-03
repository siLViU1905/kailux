#pragma once
#include "ComputePass.h"
#include "ComputePassesPushConstants.h"

namespace kailux
{
    class ComputeCuller : public ComputePass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ComputeCuller)

        static ComputeCuller create(const Context &context, uint32_t frameCount);

        template<typename... Pcs>
        void push(vk::CommandBuffer cmd, const Pcs &... pcs) const
        {
            pushImpl<kPushConstantRanges, Pcs...>(cmd, pcs...);
        }

    private:
        static constexpr std::string_view kComputeShaderPath = "shaders/cull_compute_shader.spv";

        static constexpr std::array kDescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // mesh data
                vk::ShaderStageFlagBits::eCompute
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // in indirect commands
                vk::ShaderStageFlagBits::eCompute
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // out indirect commands
                vk::ShaderStageFlagBits::eCompute
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // counter
                vk::ShaderStageFlagBits::eCompute
            )
        };
        static constexpr std::array kDescriptorPoolSizes = {
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // mesh data
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // in indirect commands
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // out indirect commands
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // counter
            ),
        };
        static_assert(
            check_descriptor_layout_bindings_and_pool_sizes_match(kDescriptorLayoutBindings, kDescriptorPoolSizes),
            "Descriptor layout bindings and pool sizes do not match"
            );

        static constexpr std::array kPushConstantRanges = {
            PushConstantRangeInfo(
                vk::ShaderStageFlagBits::eCompute,
                sizeof(ComputePassesPushConstants::CameraFrustum)
            )
        };
    };
}
