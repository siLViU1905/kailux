#include "ComputePicker.h"

namespace kailux
{
    ComputePicker::ComputePicker() = default;

    ComputePicker::ComputePicker(ComputePicker &&other) noexcept : ComputePass(std::move(other))
    {
    }

    ComputePicker &ComputePicker::operator=(ComputePicker &&other) noexcept
    {
        if (this != &other)
        {
            ComputePass::operator=(std::move(other));
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
}
