#include "ComputeCuller.h"

namespace kailux
{
    ComputeCuller::ComputeCuller() = default;

    ComputeCuller::ComputeCuller(ComputeCuller &&other) noexcept : ComputePass(std::move(other))
    {
    }

    ComputeCuller &ComputeCuller::operator=(ComputeCuller &&other) noexcept
    {
        if (this != &other)
        {
            ComputePass::operator=(std::move(other));
        }
        return *this;
    }

    ComputeCuller ComputeCuller::create(const Context &context, uint32_t frameCount)
    {
        ComputeCuller picker;
        picker.createDescriptorLayout(context, s_DescriptorLayoutBindings);
        picker.createDescriptorPool(context, frameCount, s_DescriptorPoolSizes);
        picker.createPipeline(context, {s_ComputeShaderPath.data()}, s_PushConstantRanges);
        return picker;
    }
}
