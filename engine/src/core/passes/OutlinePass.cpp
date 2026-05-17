#include "OutlinePass.h"

namespace kailux
{
    OutlinePass::OutlinePass()
    {
    }

    OutlinePass::OutlinePass(OutlinePass &&other) noexcept : m_DescriptorLayout(std::move(other.m_DescriptorLayout)),
                                                             m_DescriptorPool(std::move(other.m_DescriptorPool)),
                                                             m_Pipeline(std::move(other.m_Pipeline)),
                                                             m_Pc(other.m_Pc)
    {
    }

    OutlinePass &OutlinePass::operator=(OutlinePass &&other) noexcept
    {
        if (this != &other)
        {
            m_DescriptorLayout = std::move(other.m_DescriptorLayout);
            m_DescriptorPool = std::move(other.m_DescriptorPool);
            m_Pipeline = std::move(other.m_Pipeline);
            m_Pc = other.m_Pc;
        }
        return *this;
    }

    OutlinePass OutlinePass::create(const Context &context, const Swapchain &swapchain, uint32_t frameCount, std::string_view vertShaderPath, std::string_view fragShaderPath)
    {
        OutlinePass pass;
        pass.createDescriptorLayout(context);
        pass.createDescriptorPool(context, frameCount);
        pass.createPipeline(context, swapchain, vertShaderPath, fragShaderPath);
        return pass;
    }

    void OutlinePass::bind(vk::CommandBuffer cmd) const
    {
        m_Pipeline.bindGraphics(cmd);
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

    const DescriptorLayout &OutlinePass::getDescriptorLayout() const
    {
        return m_DescriptorLayout;
    }

    const DescriptorPool &OutlinePass::getDescriptorPool() const
    {
        return m_DescriptorPool;
    }

    const Pipeline &OutlinePass::getPipeline() const
    {
        return m_Pipeline;
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

    void OutlinePass::createDescriptorLayout(const Context &context)
    {
        m_DescriptorLayout = DescriptorLayout::create(context, s_DescriptorLayoutBindings);
    }

    void OutlinePass::createDescriptorPool(const Context &context, uint32_t frameCount)
    {
        m_DescriptorPool = DescriptorPool::create(context, frameCount, s_DescriptorLayoutSizes);
    }

    void OutlinePass::createPipeline(const Context &context, const Swapchain &swapchain,
                                     std::string_view vertShaderPath, std::string_view fragShaderPath)
    {
        m_Pipeline = Pipeline::createGraphics(
            context,
            swapchain,
            m_DescriptorLayout,
            {vertShaderPath.data(), fragShaderPath.data()},
            make_pipeline_info(swapchain),
            s_PushConstantRanges
        );
    }
}
