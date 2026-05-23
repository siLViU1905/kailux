#pragma once
#include "ComputePass.h"

namespace kailux
{
    class ComputeCuller : public ComputePass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ComputeCuller)

        static ComputeCuller create(const Context &context, uint32_t frameCount);

        void execute(vk::CommandBuffer cmd, ComputeWorkgroup group) const override;

        void setFrustum(const std::array<glm::vec4, 6>& planes, uint32_t totalObjects);

    private:
        static constexpr std::string_view s_ComputeShaderPath = "shaders/cull_compute_shader.spv";

        static constexpr std::array s_DescriptorLayoutBindings = {
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
        static constexpr std::array s_DescriptorPoolSizes = {
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
            check_descriptor_layout_bindings_and_pool_sizes_match(s_DescriptorLayoutBindings, s_DescriptorPoolSizes),
            "Descriptor layout bindings and pool sizes do not match"
            );

        struct CullerPushConstant
        {
            std::array<glm::vec4, 6> frustumPlanes{};
            uint32_t                 totalObjects{};
        };

        static constexpr std::array s_PushConstantRanges = {
            PushConstantRangeInfo(
                vk::ShaderStageFlagBits::eCompute,
                sizeof(CullerPushConstant)
            )
        };

        CullerPushConstant m_CullerPc;
    };
}
