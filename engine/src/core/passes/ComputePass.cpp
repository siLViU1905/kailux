#include "ComputePass.h"

namespace kailux
{
    ComputePass::ComputePass()
    {
    }

    ComputePass::ComputePass(ComputePass &&other) noexcept : m_DescriptorLayout(std::move(other.m_DescriptorLayout)),
                                                             m_DescriptorPool(std::move(other.m_DescriptorPool)),
                                                             m_Pipeline(std::move(other.m_Pipeline))

    {
    }

    ComputePass &ComputePass::operator=(ComputePass &&other) noexcept
    {
        if (this != &other)
        {
            m_DescriptorLayout = std::move(other.m_DescriptorLayout);
            m_DescriptorPool = std::move(other.m_DescriptorPool);
            m_Pipeline = std::move(other.m_Pipeline);
        }
        return *this;
    }

    void ComputePass::bind(vk::CommandBuffer cmd) const
    {
        m_Pipeline.bindCompute(cmd);
    }

    const DescriptorLayout &ComputePass::getDescriptorLayout() const
    {
        return m_DescriptorLayout;
    }

    const DescriptorPool &ComputePass::getDescriptorPool() const
    {
        return m_DescriptorPool;
    }

    const Pipeline &ComputePass::getPipeline() const
    {
        return m_Pipeline;
    }

    void ComputePass::createDescriptorLayout(const Context &context, std::span<const DescriptorLayoutBinding> bindings)
    {
        m_DescriptorLayout = DescriptorLayout::create(context, bindings);
    }

    void ComputePass::createDescriptorPool(const Context &context, uint32_t frameCount,
                                           std::span<const DescriptorPoolSize> sizes)
    {
        m_DescriptorPool = DescriptorPool::create(context, frameCount, sizes);
    }

    void ComputePass::createPipeline(const Context &context,
                                     const ComputeShaderInfo &info,
                                     std::span<const PushConstantRangeInfo> pushConstantRanges)
    {
        m_Pipeline = Pipeline::createCompute(
            context,
            m_DescriptorLayout,
            info,
            pushConstantRanges
        );
    }
}
