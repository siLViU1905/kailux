#include "ComputePass.h"

namespace kailux
{
    ComputePass::ComputePass()
    {
    }

    ComputePass::ComputePass(ComputePass &&other) noexcept : mDescriptorLayout(std::move(other.mDescriptorLayout)),
                                                             mDescriptorPool(std::move(other.mDescriptorPool)),
                                                             mPipeline(std::move(other.mPipeline))

    {
    }

    ComputePass &ComputePass::operator=(ComputePass &&other) noexcept
    {
        if (this != &other)
        {
            mDescriptorLayout = std::move(other.mDescriptorLayout);
            mDescriptorPool = std::move(other.mDescriptorPool);
            mPipeline = std::move(other.mPipeline);
        }
        return *this;
    }

    void ComputePass::bind(vk::CommandBuffer cmd) const
    {
        mPipeline.bindCompute(cmd);
    }

    void ComputePass::execute(vk::CommandBuffer cmd, ComputeWorkgroup group) const
    {
        cmd.dispatch(group.x, group.y, group.z);
    }

    const DescriptorLayout &ComputePass::getDescriptorLayout() const
    {
        return mDescriptorLayout;
    }

    const DescriptorPool &ComputePass::getDescriptorPool() const
    {
        return mDescriptorPool;
    }

    const Pipeline &ComputePass::getPipeline() const
    {
        return mPipeline;
    }

    void ComputePass::createDescriptorLayout(const Context &context, std::span<const DescriptorLayoutBinding> bindings)
    {
        mDescriptorLayout = DescriptorLayout::create(context, bindings);
    }

    void ComputePass::createDescriptorPool(const Context &context, uint32_t frameCount,
                                           std::span<const DescriptorPoolSize> sizes)
    {
        mDescriptorPool = DescriptorPool::create(context, frameCount, sizes);
    }

    void ComputePass::createPipeline(const Context &context,
                                     const ComputeShaderInfo &info,
                                     std::span<const PushConstantRangeInfo> pushConstantRanges)
    {
        mPipeline = Pipeline::createCompute(
            context,
            mDescriptorLayout,
            info,
            pushConstantRanges
        );
    }
}
