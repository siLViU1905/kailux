#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "Context.h"
#include "Core.h"
#include "Swapchain.h"
#include "descriptor/DescriptorLayout.h"

namespace kailux
{
    struct ShaderStageInfo
    {
        vk::ShaderStageFlagBits stage{};
        std::string             path;
    };
    using GraphicsShaderInfo = std::vector<ShaderStageInfo>;

    struct ComputeShaderInfo
    {
        std::string computeShaderPath;
    };

    struct PipelineInfo
    {
        vk::VertexInputBindingDescription                  vertexInputBinding;
        std::vector<vk::VertexInputAttributeDescription>   vertexInputAttribute;
        vk::PrimitiveTopology                              topology;
        vk::PipelineRasterizationStateCreateInfo           rasterizer;
        std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
        std::vector<vk::Format>                            colorFormats;
        vk::SampleCountFlagBits                            samples = vk::SampleCountFlagBits::e1;
        vk::PipelineDepthStencilStateCreateInfo            depthStencilInfo;
    };

    struct PushConstantRangeInfo
    {
        vk::ShaderStageFlagBits shaderStage;
        uint32_t                size{};
    };

    class Pipeline
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Pipeline)

        static Pipeline create_graphics(
            const Context &context,
            const Swapchain& swapchain,
            const DescriptorLayout& descriptorSetLayout,
            const GraphicsShaderInfo& shaderInfo,
            const PipelineInfo &pipelineInfo,
            std::span<const PushConstantRangeInfo> pushConstantRanges = {}
            );

        static Pipeline create_compute(
            const Context &context,
            const DescriptorLayout& descriptorSetLayout,
            const ComputeShaderInfo& shaderInfo,
            std::span<const PushConstantRangeInfo> pushConstantRanges = {}
            );

        void BindGraphics(vk::CommandBuffer cmd) const;
        void BindCompute(vk::CommandBuffer cmd) const;

        vk::PipelineLayout GetLayout() const;

    private:
        struct ShaderModuleInstance
        {
            vk::ShaderStageFlagBits stage;
            vk::raii::ShaderModule  module;
        };

        struct ShaderModules
        {
            std::vector<ShaderModuleInstance> modules;

            std::vector<vk::PipelineShaderStageCreateInfo> MakeVkStages() const;
        };

        static ShaderModules          create_graphics_shader_modules(const Context &context, const GraphicsShaderInfo& stages);

        void CreateLayout(const Context& context, const DescriptorLayout& descriptorSetLayout, std::span<const PushConstantRangeInfo> pushConstantRanges);
        void CreateGraphicsPipeline(const Context &context, const Swapchain& swapchain, const ShaderModules& shaderModules, const PipelineInfo& info);
        void CreateComputePipeline(const Context &context, const vk::raii::ShaderModule& shaderModule);

        vk::raii::PipelineLayout mLayout;
        vk::raii::Pipeline       mPipeline;
    };
}
