#include "OutlinePass.h"

namespace kailux
{
    OutlinePass::OutlinePass() = default;

    OutlinePass::OutlinePass(OutlinePass &&other) noexcept : GraphicsPass(std::move(other)),
                                                             m_Pc(other.m_Pc)
    {
        other.m_Pc = {};
    }

    OutlinePass &OutlinePass::operator=(OutlinePass &&other) noexcept
    {
        if (this != &other)
        {
            GraphicsPass::operator=(std::move(other));
            m_Pc = other.m_Pc;

            other.m_Pc = {};
        }
        return *this;
    }

    OutlinePass OutlinePass::create(const Context &context, const Swapchain &swapchain, uint32_t frameCount)
    {
        OutlinePass pass;
        pass.createDescriptorLayout(context, s_DescriptorLayoutBindings);
        pass.createDescriptorPool(context, frameCount, s_DescriptorLayoutSizes);
        pass.createPipeline(context, swapchain, s_OutlineVertexShaderPath, s_OutlineFragmentShaderPath, make_pipeline_info(swapchain), s_PushConstantRanges);
        return pass;
    }

    void OutlinePass::push(vk::CommandBuffer cmd) const
    {
        cmd.pushConstants(
            m_Pipeline.getLayout(),
            vk::ShaderStageFlagBits::eFragment,
            0,
            sizeof(OutlinePushConstant),
            &m_Pc
            );
    }

    void OutlinePass::setColorAndId(glm::vec3 color, uint32_t id)
    {
        m_Pc = {{color, 0.f}, id};
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
