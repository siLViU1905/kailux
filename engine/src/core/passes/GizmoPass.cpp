#include "GizmoPass.h"
#include "../gizmo/GizmoVertex.h"

namespace kailux
{
    GizmoPass::GizmoPass() = default;

    GizmoPass::GizmoPass(GizmoPass &&other) noexcept : GraphicsPass(std::move(other))
    {
    }

    GizmoPass &GizmoPass::operator=(GizmoPass &&other) noexcept
    {
        if (this != &other)
        {
            GraphicsPass::operator=(std::move(other));
        }
        return *this;
    }

    GizmoPass GizmoPass::create(const Context &context, const Swapchain &swapchain, uint32_t maxFrames)
    {
        GizmoPass pass;
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
        return pass;
    }

    PipelineInfo GizmoPass::make_pipeline_info(const Swapchain &swapchain, vk::SampleCountFlagBits sampleCount)
    {
        PipelineInfo info;
        info.vertexInputBinding = GizmoVertex::get_binding_description();
        constexpr auto vertexAttribDesc = GizmoVertex::get_attribute_description();
        info.vertexInputAttribute = {vertexAttribDesc.cbegin(), vertexAttribDesc.cend()};
        info.topology = vk::PrimitiveTopology::eTriangleList;

        info.rasterizer = {
            {},
            vk::False,
            vk::False,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eNone,
            vk::FrontFace::eCounterClockwise,
            vk::False, {}, {}, 1.f,
            1.f
        };

        vk::PipelineColorBlendAttachmentState colorAttachment{};
        colorAttachment.colorWriteMask =
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
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
        idAttachment.colorWriteMask = {};
        idAttachment.blendEnable = vk::False;
        info.colorBlendAttachments.push_back(idAttachment);
        info.colorFormats.push_back(vk::Format::eR32Uint);

        info.samples = sampleCount;

        info.depthStencilInfo.depthTestEnable = vk::True;
        info.depthStencilInfo.depthWriteEnable = vk::False;
        info.depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
        info.depthStencilInfo.depthBoundsTestEnable = vk::False;
        info.depthStencilInfo.stencilTestEnable = vk::False;

        return info;
    }
}
