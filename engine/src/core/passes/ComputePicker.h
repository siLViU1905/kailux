#pragma once
#include "ComputePass.h"
#include "ComputePassesPushConstants.h"
#include "../Pipeline.h"
#include "../descriptor/DescriptorLayout.h"
#include "../descriptor/DescriptorPool.h"

namespace kailux
{
    class ComputePicker : public ComputePass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ComputePicker)

        static ComputePicker create(const Context &context, uint32_t frameCount);

        template<typename... Pcs>
        void push(vk::CommandBuffer cmd, const Pcs &... pcs) const
        {
            pushImpl<kPushConstantRanges, Pcs...>(cmd, pcs...);
        }

    private:
        static constexpr std::string_view kPickerComputeShaderPath = "shaders/entity_picker_compute_shader.spv";

        static constexpr std::array kDescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageImage,
                1, // in id's image
                vk::ShaderStageFlagBits::eCompute
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // out id
                vk::ShaderStageFlagBits::eCompute
            )
        };
        static constexpr std::array kDescriptorPoolSizes = {
            DescriptorPoolSize(
                vk::DescriptorType::eStorageImage,
                1 // id's image
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // out id
            )
        };
        static_assert(
            check_descriptor_layout_bindings_and_pool_sizes_match(kDescriptorLayoutBindings, kDescriptorPoolSizes),
            "Descriptor layout bindings and pool sizes do not match"
            );

        static constexpr std::array kPushConstantRanges = {
            PushConstantRangeInfo(
                vk::ShaderStageFlagBits::eCompute,
                sizeof(ComputePassesPushConstants::MouseCords)
            )
        };
    };
}
