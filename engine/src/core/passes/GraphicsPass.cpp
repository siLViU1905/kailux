#include "GraphicsPass.h"

namespace kailux
{
    GraphicsPass::GraphicsPass()
    {
    }

    GraphicsPass::GraphicsPass(GraphicsPass &&other) noexcept : mDescriptorLayout(std::move(other.mDescriptorLayout)),
                                                                mDescriptorPool(std::move(other.mDescriptorPool)),
                                                                mPipeline(std::move(other.mPipeline))

    {
    }

    GraphicsPass &GraphicsPass::operator=(GraphicsPass &&other) noexcept
    {
        if (this != &other)
        {
            mDescriptorLayout = std::move(other.mDescriptorLayout);
            mDescriptorPool = std::move(other.mDescriptorPool);
            mPipeline = std::move(other.mPipeline);
        }
        return *this;
    }

    void GraphicsPass::bind(vk::CommandBuffer cmd) const
    {
        mPipeline.bindGraphics(cmd);
    }

    const DescriptorLayout &GraphicsPass::getDescriptorLayout() const
    {
        return mDescriptorLayout;
    }

    const DescriptorPool &GraphicsPass::getDescriptorPool() const
    {
        return mDescriptorPool;
    }

    const Pipeline &GraphicsPass::getPipeline() const
    {
        return mPipeline;
    }

    void GraphicsPass::createDescriptorLayout(const Context &context, std::span<const DescriptorLayoutBinding> bindings)
    {
        mDescriptorLayout = DescriptorLayout::create(context, bindings);
    }

    void GraphicsPass::createDescriptorPool(const Context &context, uint32_t frameCount,
                                            std::span<const DescriptorPoolSize> sizes)
    {
        mDescriptorPool = DescriptorPool::create(context, frameCount, sizes);
    }

    void GraphicsPass::createPipeline(const Context &context, const Swapchain &swapchain,
                                      std::string_view vertShaderPath, std::string_view fragShaderPath,
                                      const PipelineInfo &info,
                                      std::span<const PushConstantRangeInfo> pushConstantRanges)
    {
        GraphicsShaderInfo shaderInfo;
        if (!vertShaderPath.empty())
            shaderInfo.emplace_back(vk::ShaderStageFlagBits::eVertex, vertShaderPath.data());
        if (!fragShaderPath.empty())
            shaderInfo.emplace_back(vk::ShaderStageFlagBits::eFragment, fragShaderPath.data());

        mPipeline = Pipeline::createGraphics(
            context,
            swapchain,
            mDescriptorLayout,
            shaderInfo,
            info,
            pushConstantRanges
        );
    }
}
