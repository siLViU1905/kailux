#include "ShadowPass.h"

#include "core/mesh/Vertex.h"

namespace kailux
{
    ShadowPass::ShadowPass() = default;

    ShadowPass::ShadowPass(ShadowPass &&other) noexcept : GraphicsPass(std::move(other))
    {
    }

    ShadowPass &ShadowPass::operator=(ShadowPass &&other) noexcept
    {
        if (this != &other)
            GraphicsPass::operator=(std::move(other));
        return *this;
    }

    ShadowPass ShadowPass::create(const Context &context, const Swapchain &swapchain, uint32_t maxFrames)
    {
        ShadowPass pass;
        pass.CreateDescriptorLayout(context, kDescriptorLayoutBindings);
        pass.CreateDescriptorPool(context, maxFrames, kDescriptorPoolSizes);
        pass.CreatePipeline(
            context,
            swapchain,
            kVertexShaderPath,
            {},
            make_pipeline_info(),
            kPushConstantRanges
        );
        return pass;
    }

    PipelineInfo ShadowPass::make_pipeline_info()
    {
        PipelineInfo info;
        info.vertexInputBinding = Vertex::get_binding_description();
        constexpr auto vertexAttribDesc = Vertex::get_attribute_description();
        info.vertexInputAttribute = {vertexAttribDesc[0]};

        info.topology = vk::PrimitiveTopology::eTriangleList;

        info.rasterizer = {
                {},
                vk::False,
                vk::False,
                vk::PolygonMode::eFill,
                vk::CullModeFlagBits::eNone,
                vk::FrontFace::eCounterClockwise,
                vk::True,
                1.25f,
                0.f,
                1.75f,
                1.f
            };

        info.samples = vk::SampleCountFlagBits::e1;

        info.depthStencilInfo.depthTestEnable = vk::True;
        info.depthStencilInfo.depthWriteEnable = vk::True;
        info.depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
        info.depthStencilInfo.depthBoundsTestEnable = vk::False;
        info.depthStencilInfo.stencilTestEnable = vk::False;

        return info;
    }
}
