#include "ComputeShadowCuller.h"
#include "Shader.h"

namespace kailux
{
    ComputeShadowCuller::ComputeShadowCuller() = default;

    ComputeShadowCuller::ComputeShadowCuller(ComputeShadowCuller &&other) noexcept : ComputePass(std::move(other))
    {
    }

    ComputeShadowCuller &ComputeShadowCuller::operator=(ComputeShadowCuller &&other) noexcept
    {
        if (this != &other)
        {
            ComputePass::operator=(std::move(other));
        }
        return *this;
    }
    
    ComputeShadowCuller ComputeShadowCuller::create(const Context &context, uint32_t frameCount)
    {
        ComputeShadowCuller culler;
        culler.CreateDescriptorLayout(context, kDescriptorLayoutBindings);
        culler.CreateDescriptorPool(context, frameCount, kDescriptorPoolSizes);
        culler.CreatePipeline(context, {kComputeShaderPath.data()}, kPushConstantRanges);
        return culler;
    }
}