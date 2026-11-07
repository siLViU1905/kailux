#pragma once
#include "ComputePass.h"
#include "ComputePassesPushConstants.h"

namespace kailux
{
    struct ShadowCullTarget
    {
        std::array<glm::vec4, 6> planes{};
        glm::uvec4               info{}; // x - enabled
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(ShadowCullTarget)

    class ComputeShadowCuller : public ComputePass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ComputeShadowCuller)

        static ComputeShadowCuller create(const Context &context, uint32_t frameCount);

        template<typename... Pcs>
        void Push(vk::CommandBuffer cmd, const Pcs &... pcs) const
        {
            PushImpl<kPushConstantRanges, Pcs...>(cmd, pcs...);
        }

        static constexpr uint32_t kWorkgroupSize{256};
    
    private:
        static constexpr std::string_view kComputeShaderPath = "shaders/shadow_cull_compute_shader.glsl";

        static constexpr std::array kDescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // mesh data
                vk::ShaderStageFlagBits::eCompute
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // targets
                vk::ShaderStageFlagBits::eCompute
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // shadow commands, one per (list, batch)
                vk::ShaderStageFlagBits::eCompute
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // out instance ids
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
                1 // targets
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // shadow commands
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // out instance ids
            )
        };
        static_assert(
            check_descriptor_layout_bindings_and_pool_sizes_match(kDescriptorLayoutBindings, kDescriptorPoolSizes),
            "Descriptor layout bindings and pool sizes do not match"
            );
            
        static constexpr std::array kPushConstantRanges = {
            PushConstantRangeInfo(
                vk::ShaderStageFlagBits::eCompute,
                sizeof(compute_pass_push_constants::ShadowCullParams)
            )
        };
    };
}