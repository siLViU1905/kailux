#pragma once
#include "Pipeline.h"

namespace kailux
{
    struct ComputeShaderInfo
    {
        std::string computeShaderPath;
    };

    class ComputePipeline final : public Pipeline<ComputePipeline>
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ComputePipeline)

        static ComputePipeline create(
            const Context &context,
            const DescriptorLayout &descriptorSetLayout,
            const ComputeShaderInfo &shaderInfo,
            std::span<const PushConstantRangeInfo> pushConstantRanges = {}
        );

        template <class Derived>
        friend class Pipeline;

        static constexpr auto kBindPoint{vk::PipelineBindPoint::eCompute};

    private:
        ComputePipeline(vk::raii::PipelineLayout &&layout, vk::raii::Pipeline &&pipeline);

        using PBase = Pipeline<ComputePipeline>;

        static vk::raii::ShaderModule create_compute_shader_module(const Context &context, const ComputeShaderInfo& info);

        static vk::raii::Pipeline create_pipeline_impl(
            const Context &context,
            const vk::raii::PipelineLayout &layout,
            const vk::raii::ShaderModule &shaderModule
        );
    };
}
