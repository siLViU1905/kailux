#include "ParticlePass.h"
#include "Shader.h"

namespace kailux
{
    ParticlePass::ParticlePass() = default;

    ParticlePass::ParticlePass(ParticlePass &&other) noexcept : GraphicsPass(std::move(other)),
                                                                mMultisampledPipeline(std::move(other.mMultisampledPipeline))
    {
    }

    ParticlePass &ParticlePass::operator=(ParticlePass &&other) noexcept
    {
        if (this != &other)
        {
            GraphicsPass::operator=(std::move(other));
            mMultisampledPipeline = std::move(other.mMultisampledPipeline);
        }
        return *this;
    }

    ParticlePass ParticlePass::create(const Context &context, const Swapchain &swapchain, uint32_t maxFrames)
    {
        ParticlePass pass;
        pass.CreateDescriptorLayout(context, kDescriptorLayoutBindings);
        pass.CreateDescriptorPool(context, maxFrames, kDescriptorPoolSizes);

        GraphicsShaderInfo shaderInfo;
        shaderInfo.emplace_back(vk::ShaderStageFlagBits::eVertex, kVertexShaderPath.data());
        shaderInfo.emplace_back(vk::ShaderStageFlagBits::eFragment, kFragmentShaderPath.data());

        pass.CreatePipeline(
            context,
            swapchain,
            shaderInfo,
            make_pipeline_info(swapchain, vk::SampleCountFlagBits::e1, true),
            kPushConstantRanges
        );

        pass.mMultisampledPipeline = GraphicsPipeline::create(
            context,
            swapchain,
            pass.mDescriptorLayout,
            shaderInfo,
            make_pipeline_info(swapchain, context.GetMaxUsableSampleCount(), false),
            kPushConstantRanges
        );
        return pass;
    }

    void ParticlePass::Bind(vk::CommandBuffer cmd, bool multisampled) const
    {
        multisampled ? mMultisampledPipeline.Bind(cmd) : mPipeline.Bind(cmd);
    }

    PipelineInfo ParticlePass::make_pipeline_info(const Swapchain &swapchain, vk::SampleCountFlagBits samples, bool writeIds)
    {
        PipelineInfo info;
        info.vertexInputBinding = vk::VertexInputBindingDescription{0, 0, vk::VertexInputRate::eVertex};
        info.vertexInputAttribute = {};
        info.topology = vk::PrimitiveTopology::eTriangleList;

        info.rasterizer = {
            {},
            vk::False,
            vk::False,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eNone,
            vk::FrontFace::eCounterClockwise,
            vk::False, {}, {}, {},
            1.f
        };

        vk::PipelineColorBlendAttachmentState colorAttachment{};
        colorAttachment.colorWriteMask =
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colorAttachment.blendEnable = vk::True;
        colorAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
        colorAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        colorAttachment.colorBlendOp = vk::BlendOp::eAdd;
        colorAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        colorAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        colorAttachment.alphaBlendOp = vk::BlendOp::eAdd;
        info.colorBlendAttachments.push_back(colorAttachment);
        info.colorFormats.push_back(swapchain.GetFormat());

        if (writeIds)
        {
            vk::PipelineColorBlendAttachmentState idAttachment{};
            idAttachment.colorWriteMask = {};
            idAttachment.blendEnable = vk::False;
            info.colorBlendAttachments.push_back(idAttachment);
            info.colorFormats.push_back(vk::Format::eR32Uint);
        }

        info.samples = samples;

        info.depthStencilInfo.depthTestEnable = vk::True;
        info.depthStencilInfo.depthWriteEnable = vk::False;
        info.depthStencilInfo.depthCompareOp = vk::CompareOp::eLessOrEqual;
        info.depthStencilInfo.depthBoundsTestEnable = vk::False;
        info.depthStencilInfo.stencilTestEnable = vk::False;

        return info;
    }
}