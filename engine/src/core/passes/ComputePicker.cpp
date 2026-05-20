#include "ComputePicker.h"

namespace kailux
{
    ComputePicker::ComputePicker() = default;

    ComputePicker::ComputePicker(ComputePicker &&other) noexcept : ComputePass(std::move(other)),
                                                                   m_Cords(other.m_Cords)
    {
    }

    ComputePicker &ComputePicker::operator=(ComputePicker &&other) noexcept
    {
        if (this != &other)
        {
            ComputePass::operator=(std::move(other));
            m_Cords = other.m_Cords;
        }
        return *this;
    }

    ComputePicker ComputePicker::create(const Context &context, uint32_t frameCount)
    {
        ComputePicker picker;
        picker.createDescriptorLayout(context, s_DescriptorLayoutBindings);
        picker.createDescriptorPool(context, frameCount, s_DescriptorPoolSizes);
        picker.createPipeline(context, {s_PickerComputeShaderPath.data()}, s_PushConstantRanges);
        return picker;
    }

    void ComputePicker::execute(vk::CommandBuffer cmd, ComputeWorkgroup group) const
    {
        cmd.pushConstants(
            m_Pipeline.getLayout(),
            vk::ShaderStageFlagBits::eCompute,
            0,
            sizeof(MouseCords),
            &m_Cords
        );
        cmd.dispatch(group.x, group.y, group.z);
    }

    void ComputePicker::setCords(uint32_t x, uint32_t y)
    {
        m_Cords.x = x;
        m_Cords.y = y;
    }
}
