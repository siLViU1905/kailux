#include "ComputeParticle.h"
#include "Shader.h"

namespace kailux
{
    ComputeParticle::ComputeParticle() = default;

    ComputeParticle::ComputeParticle(ComputeParticle &&other) noexcept : ComputePass(std::move(other))
    {
    }

    ComputeParticle &ComputeParticle::operator=(ComputeParticle &&other) noexcept
    {
        if (this != &other)
        {
            ComputePass::operator=(std::move(other));
        }
        return *this;
    }

    ComputeParticle ComputeParticle::create(const Context &context, uint32_t frameCount)
    {
        ComputeParticle compute;
        compute.CreateDescriptorLayout(context, kDescriptorLayoutBindings);
        compute.CreateDescriptorPool(context, frameCount, kDescriptorPoolSizes);
        compute.CreatePipeline(context, {kComputeShaderPath.data()}, kPushConstantRanges);
        return compute;
    }
}