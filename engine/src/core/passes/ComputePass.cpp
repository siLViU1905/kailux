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

    void ComputePass::Bind(vk::CommandBuffer cmd) const
    {
        mPipeline.BindCompute(cmd);
    }

    void ComputePass::Execute(vk::CommandBuffer cmd, ComputeWorkgroup group) const
    {
        cmd.dispatch(group.x, group.y, group.z);
    }

    const DescriptorLayout &ComputePass::GetDescriptorLayout() const
    {
        return mDescriptorLayout;
    }

    const DescriptorPool &ComputePass::GetDescriptorPool() const
    {
        return mDescriptorPool;
    }

    const Pipeline &ComputePass::GetPipeline() const
    {
        return mPipeline;
    }

    void ComputePass::CreateDescriptorLayout(const Context &context, std::span<const DescriptorLayoutBinding> bindings)
    {
        mDescriptorLayout = DescriptorLayout::create(context, bindings);
    }

    void ComputePass::CreateDescriptorPool(const Context &context, uint32_t frameCount,
                                           std::span<const DescriptorPoolSize> sizes)
    {
        mDescriptorPool = DescriptorPool::create(context, frameCount, sizes);
    }

    void ComputePass::CreatePipeline(const Context &context,
                                     const ComputeShaderInfo &info,
                                     std::span<const PushConstantRangeInfo> pushConstantRanges)
    {
        mPipeline = Pipeline::create_compute(
            context,
            mDescriptorLayout,
            info,
            pushConstantRanges
        );
    }
}
