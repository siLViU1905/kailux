#include "OutlinePass.h"

namespace kailux
{
    OutlinePass::OutlinePass() = default;

    OutlinePass::OutlinePass(OutlinePass &&other) noexcept : GraphicsPass(std::move(other))
    {
    }

    OutlinePass &OutlinePass::operator=(OutlinePass &&other) noexcept
    {
        if (this != &other)
        {
            GraphicsPass::operator=(std::move(other));
        }
        return *this;
    }

    OutlinePass OutlinePass::create(const Context &context, const Swapchain &swapchain, uint32_t frameCount)
    {
        OutlinePass pass;
        pass.createDescriptorLayout(context, kDescriptorLayoutBindings);
        pass.createDescriptorPool(context, frameCount, kDescriptorPoolSizes);
        pass.createPipeline(context, swapchain, kOutlineVertexShaderPath, kOutlineFragmentShaderPath, make_pipeline_info(swapchain), kPushConstantRanges);
        return pass;
    }

    PipelineInfo OutlinePass::make_pipeline_info(const Swapchain &swapchain)
    {
        PipelineInfo info{};
        
        info.vertexInputAttribute.clear();
        
        info.topology = vk::PrimitiveTopology::eTriangleList;
        
        info.rasterizer.depthClampEnable = vk::False;
        info.rasterizer.rasterizerDiscardEnable = vk::False;
        info.rasterizer.polygonMode = vk::PolygonMode::eFill;
        info.rasterizer.lineWidth = 1.0f;
        info.rasterizer.cullMode = vk::CullModeFlagBits::eNone;
        info.rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
        info.rasterizer.depthBiasEnable = vk::False;
        
        vk::PipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
                                         vk::ColorComponentFlagBits::eG |
                                         vk::ColorComponentFlagBits::eB |
                                         vk::ColorComponentFlagBits::eA;
        blendAttachment.blendEnable = vk::True;
        blendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        blendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        blendAttachment.colorBlendOp = vk::BlendOp::eAdd;
        blendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        blendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        blendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

        info.colorBlendAttachments = {blendAttachment};
        
        info.colorFormats = {swapchain.getFormat()};
        info.samples = vk::SampleCountFlagBits::e1;
        
        info.depthStencilInfo.depthTestEnable = vk::False;
        info.depthStencilInfo.depthWriteEnable = vk::False;
        info.depthStencilInfo.depthCompareOp = vk::CompareOp::eAlways;
        info.depthStencilInfo.depthBoundsTestEnable = vk::False;
        info.depthStencilInfo.stencilTestEnable = vk::False;

        return info;
    }
}
