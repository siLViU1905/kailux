#include "ComputeCompactor.h"
#include "Shader.h"

namespace kailux
{
    ComputeCompactor::ComputeCompactor() = default;

    ComputeCompactor::ComputeCompactor(ComputeCompactor &&other) noexcept : ComputePass(std::move(other))
    {
    }

    ComputeCompactor &ComputeCompactor::operator=(ComputeCompactor &&other) noexcept
    {
        if (this != &other)
        {
            ComputePass::operator=(std::move(other));
        }
        return *this;
    }

    ComputeCompactor ComputeCompactor::create(const Context &context, uint32_t frameCount)
    {
        ComputeCompactor compactor;
        compactor.CreateDescriptorLayout(context, kDescriptorLayoutBindings);
        compactor.CreateDescriptorPool(context, frameCount, kDescriptorPoolSizes);
        compactor.CreatePipeline(context, {kComputeShaderPath.data()}, kPushConstantRanges);
        return compactor;
    }
}