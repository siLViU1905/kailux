#pragma once
#include "ComputePass.h"
#include "ComputePassesPushConstants.h"

namespace kailux
{
    class ComputeCompactor : public ComputePass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ComputeCompactor)

        static ComputeCompactor create(const Context& context, uint32_t frameCount);

        template<typename... Pcs>
        void Push(vk::CommandBuffer cmd, const Pcs&... pcs) const
        {
            PushImpl<kPushConstantRanges, Pcs...>(cmd, pcs...);
        }

        static constexpr uint32_t kWorkgroupSize{256};

    private:
        static constexpr std::string_view kComputeShaderPath = "shaders/compact_commands_compute_shader.glsl";

        static constexpr std::array kDescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // batch commands
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
                1 // batch commands
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // out indirect commands
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // counter
            )
        };
        static_assert(
            check_descriptor_layout_bindings_and_pool_sizes_match(kDescriptorLayoutBindings, kDescriptorPoolSizes),
            "Descriptor layout bindings and pool sizes do not match"
            );

        static constexpr std::array kPushConstantRanges = {
            PushConstantRangeInfo(
                vk::ShaderStageFlagBits::eCompute,
                sizeof(compute_pass_push_constants::CompactParams)
            )
        };
    };
}