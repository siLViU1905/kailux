#include "MainPass.h"

#include "core/mesh/Vertex.h"

namespace kailux
{
    MainPass::MainPass() = default;

    MainPass::MainPass(MainPass &&other) noexcept : GraphicsPass(std::move(other))
    {
    }

    MainPass & MainPass::operator=(MainPass &&other) noexcept
    {
        if (this != &other)
        {
            GraphicsPass::operator=(std::move(other));
        }
        return *this;
    }

    MainPass MainPass::create(const Context &context, const Swapchain &swapchain, uint32_t maxFrames)
    {
        MainPass pass;
        pass.createDescriptorLayout(context, s_DescriptorLayoutBindings);
        pass.createDescriptorPool(context, maxFrames, s_DescriptorPoolSizes);
        pass.createPipeline(
            context,
            swapchain,
            s_VertexShaderPath,
            s_FragmentShaderPath,
            make_pipeline_info(swapchain, context.getMaxUsableSampleCount()),
            {}
            );
        return pass;
    }

    void MainPass::push(vk::CommandBuffer cmd) const
    {
        return;
    }

    PipelineInfo MainPass::make_pipeline_info(const Swapchain &swapchain, vk::SampleCountFlagBits sampleCount)
    {
        PipelineInfo info;

        info.vertexInputBinding = Vertex::get_binding_description();
        constexpr auto vertexAttribDesc = Vertex::get_attribute_description();
        info.vertexInputAttribute = {vertexAttribDesc.cbegin(), vertexAttribDesc.cend()};

        info.topology = vk::PrimitiveTopology::eTriangleList;

        info.rasterizer = {
            {},
            vk::False,
            vk::False,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eCounterClockwise,
            vk::False,
            {},
            {},
            1.f,
            1.f
        };

        vk::PipelineColorBlendAttachmentState colorAttachment{};
        colorAttachment.colorWriteMask =
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eA;
        colorAttachment.blendEnable = vk::True;
        colorAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        colorAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        colorAttachment.colorBlendOp = vk::BlendOp::eAdd;
        colorAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        colorAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        colorAttachment.alphaBlendOp = vk::BlendOp::eAdd;

        info.colorBlendAttachments.push_back(colorAttachment);
        info.colorFormats.push_back(swapchain.getFormat());

        vk::PipelineColorBlendAttachmentState idAttachment{};
        idAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR;
        idAttachment.blendEnable = vk::False;

        info.colorBlendAttachments.push_back(idAttachment);
        info.colorFormats.push_back(vk::Format::eR32Uint);

        info.samples = sampleCount;

        info.depthStencilInfo.depthTestEnable = vk::True;
        info.depthStencilInfo.depthWriteEnable = vk::True;
        info.depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
        info.depthStencilInfo.depthBoundsTestEnable = vk::False;
        info.depthStencilInfo.stencilTestEnable = vk::False;

        return info;
    }
}