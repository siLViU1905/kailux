#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "Context.h"
#include "Core.h"
#include "Swapchain.h"
#include "descriptor/DescriptorLayout.h"

namespace kailux
{
    struct GraphicsShaderInfo
    {
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
    };

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

        static Pipeline createGraphics(
            const Context &context,
            const Swapchain& swapchain,
            const DescriptorLayout& descriptorSetLayout,
            const GraphicsShaderInfo& shaderInfo,
            const PipelineInfo &pipelineInfo,
            std::span<const PushConstantRangeInfo> pushConstantRanges = {}
            );

        static Pipeline createCompute(
            const Context &context,
            const DescriptorLayout& descriptorSetLayout,
            const ComputeShaderInfo& shaderInfo,
            std::span<const PushConstantRangeInfo> pushConstantRanges = {}
            );

        void bindGraphics(vk::CommandBuffer cmd) const;
        void bindCompute(vk::CommandBuffer cmd) const;

        vk::PipelineLayout getLayout() const;

    private:
        struct ShaderModules
        {
            vk::raii::ShaderModule vertex;
            vk::raii::ShaderModule fragment;

            std::array<vk::PipelineShaderStageCreateInfo, 2> getStages() const;
        };

        static std::vector<char>      read_shader_from_file(std::string_view path);
        static vk::raii::ShaderModule create_shader_module(const Context &context, const std::vector<char> &code);
        static ShaderModules          create_graphics_shader_modules(const Context &context, const GraphicsShaderInfo& info);

        void createLayout(const Context& context, const DescriptorLayout& descriptorSetLayout, std::span<const PushConstantRangeInfo> pushConstantRanges);
        void createGraphicsPipeline(const Context &context, const Swapchain& swapchain, const ShaderModules& shaderModules, const PipelineInfo& info);
        void createComputePipeline(const Context &context, const vk::raii::ShaderModule& shaderModule);

        vk::raii::PipelineLayout m_Layout;
        vk::raii::Pipeline       m_Pipeline;
    };
}
