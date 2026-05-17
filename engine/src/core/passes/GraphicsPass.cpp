#include "GraphicsPass.h"

namespace kailux
{
    GraphicsPass::GraphicsPass()
    {
    }

    GraphicsPass::GraphicsPass(GraphicsPass &&other) noexcept : m_DescriptorLayout(std::move(other.m_DescriptorLayout)),
                                                                m_DescriptorPool(std::move(other.m_DescriptorPool)),
                                                                m_Pipeline(std::move(other.m_Pipeline))

    {
    }

    GraphicsPass &GraphicsPass::operator=(GraphicsPass &&other) noexcept
    {
        if (this != &other)
        {
            m_DescriptorLayout = std::move(other.m_DescriptorLayout);
            m_DescriptorPool = std::move(other.m_DescriptorPool);
            m_Pipeline = std::move(other.m_Pipeline);
        }
        return *this;
    }

    void GraphicsPass::bind(vk::CommandBuffer cmd) const
    {
        m_Pipeline.bindGraphics(cmd);
    }

    const DescriptorLayout &GraphicsPass::getDescriptorLayout() const
    {
        return m_DescriptorLayout;
    }

    const DescriptorPool &GraphicsPass::getDescriptorPool() const
    {
        return m_DescriptorPool;
    }

    const Pipeline &GraphicsPass::getPipeline() const
    {
        return m_Pipeline;
    }

    void GraphicsPass::createDescriptorLayout(const Context &context, std::span<const DescriptorLayoutBinding> bindings)
    {
        m_DescriptorLayout = DescriptorLayout::create(context, bindings);
    }

    void GraphicsPass::createDescriptorPool(const Context &context, uint32_t frameCount,
                                            std::span<const DescriptorPoolSize> sizes)
    {
        m_DescriptorPool = DescriptorPool::create(context, frameCount, sizes);
    }

    void GraphicsPass::createPipeline(const Context &context, const Swapchain &swapchain,
                                      std::string_view vertShaderPath, std::string_view fragShaderPath,
                                      const PipelineInfo &info,
                                      std::span<const PushConstantRangeInfo> pushConstantRanges)
    {
        m_Pipeline = Pipeline::createGraphics(
            context,
            swapchain,
            m_DescriptorLayout,
            {vertShaderPath.data(), fragShaderPath.data()},
            info,
            pushConstantRanges
        );
    }
}
