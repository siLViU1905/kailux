#pragma once
#include "Pipeline.h"

namespace kailux
{
    class GraphicsPipeline final : public Pipeline<GraphicsPipeline>
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(GraphicsPipeline)

        static GraphicsPipeline create(
            const Context &context,
            const Swapchain &swapchain,
            const DescriptorLayout &descriptorSetLayout,
            const GraphicsShaderInfo &shaderInfo,
            const PipelineInfo &pipelineInfo,
            std::span<const PushConstantRangeInfo> pushConstantRanges = {}
        );

        template <class Derived>
        friend class Pipeline;

        static constexpr auto kBindPoint{vk::PipelineBindPoint::eGraphics};

    private:
        GraphicsPipeline(vk::raii::PipelineLayout &&layout, vk::raii::Pipeline &&pipeline);

        using PBase = Pipeline<GraphicsPipeline>;

        static ShaderModules create_graphics_shader_modules(const Context &context, const GraphicsShaderInfo& stages);

        static vk::raii::Pipeline create_pipeline_impl(
            const Context &context,
            const vk::raii::PipelineLayout &layout,
            const Swapchain &swapchain,
            const ShaderModules &shaderModules,
            const PipelineInfo &info
        );
    };
}
