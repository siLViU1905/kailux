#include "MainPass.h"

#include "core/mesh/Vertex.h"

namespace kailux
{
    MainPass::MainPass() = default;

    MainPass::MainPass(MainPass &&other) noexcept : GraphicsPass(std::move(other)), mNoIdPipeline(std::move(other.mNoIdPipeline))
    {
    }

    MainPass & MainPass::operator=(MainPass &&other) noexcept
    {
        if (this != &other)
        {
            GraphicsPass::operator=(std::move(other));
            mNoIdPipeline = std::move(other.mNoIdPipeline);
        }
        return *this;
    }

    MainPass MainPass::create(const Context &context, const Swapchain &swapchain, uint32_t maxFrames)
    {
        MainPass pass;
        pass.createDescriptorLayout(context, kDescriptorLayoutBindings);
        pass.createDescriptorPool(context, maxFrames, kDescriptorPoolSizes);
        pass.createPipeline(
            context,
            swapchain,
            kVertexShaderPath,
            kFragmentShaderPath,
            make_pipeline_info(swapchain, context.getMaxUsableSampleCount()),
            kPushConstantRanges
            );
        pass.createNoIdPipeline(
            context,
            swapchain,
            kVertexShaderPath,
            kFragmentShaderPath,
            make_no_id_pipeline_info(swapchain, context.getMaxUsableSampleCount()),
            kPushConstantRanges
            );
        return pass;
    }

    void MainPass::bind(vk::CommandBuffer cmd, bool writeIds) const
    {
        writeIds ? mPipeline.bindGraphics(cmd) : mNoIdPipeline.bindGraphics(cmd);
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

    PipelineInfo MainPass::make_no_id_pipeline_info(const Swapchain &swapchain, vk::SampleCountFlagBits sampleCount)
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

        info.samples = sampleCount;

        info.depthStencilInfo.depthTestEnable = vk::True;
        info.depthStencilInfo.depthWriteEnable = vk::True;
        info.depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
        info.depthStencilInfo.depthBoundsTestEnable = vk::False;
        info.depthStencilInfo.stencilTestEnable = vk::False;

        return info;
    }

    void MainPass::createNoIdPipeline(const Context &context, const Swapchain &swapchain,
        std::string_view vertShaderPath, std::string_view fragShaderPath, const PipelineInfo &info,
        std::span<const PushConstantRangeInfo> pushConstantRanges)
    {
        GraphicsShaderInfo shaderInfo;
        if (!vertShaderPath.empty())
            shaderInfo.emplace_back(vk::ShaderStageFlagBits::eVertex, vertShaderPath.data());
        if (!fragShaderPath.empty())
            shaderInfo.emplace_back(vk::ShaderStageFlagBits::eFragment, fragShaderPath.data());

        mNoIdPipeline = Pipeline::createGraphics(
            context,
            swapchain,
            mDescriptorLayout,
            shaderInfo,
            info,
            pushConstantRanges
        );
    }
}
