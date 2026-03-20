#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "Context.h"
#include "Core.h"

namespace kailux
{
    struct ShaderInfo
    {
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
    };

    struct PipelineInfo
    {
        vk::PrimitiveTopology                    topology;
        vk::PipelineRasterizationStateCreateInfo rasterizer;
        vk::PipelineColorBlendAttachmentState    colorBlendAttachment;
        vk::SampleCountFlagBits                  samples = vk::SampleCountFlagBits::e1;
        vk::PipelineDepthStencilStateCreateInfo  depthStencilInfo;
    };

    class Pipeline
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Pipeline)

        static Pipeline create(const Context &context, const Swapchain& swapchain, const DescriptorSetLayout& descriptorSetLayout, const ShaderInfo& shaderInfo, const PipelineInfo &pipelineInfo);

    private:
        struct ShaderModules
        {
            vk::raii::ShaderModule vertex;
            vk::raii::ShaderModule fragment;

            std::array<vk::PipelineShaderStageCreateInfo, 2> getStages() const;
        };

        static std::vector<char> read_shader_from_file(std::string_view path);
        static vk::raii::ShaderModule create_shader_module(const Context &context, const std::vector<char> &code);
        static ShaderModules create_shader_modules(const Context &context, const ShaderInfo& info);

        void createLayout(const Context& context, const DescriptorSetLayout& descriptorSetLayout);
        void createPipeline(const Context &context, const Swapchain& swapchain, const ShaderModules& shaderModules, const PipelineInfo& info);

        vk::raii::PipelineLayout m_Layout;
        vk::raii::Pipeline       m_Pipeline;
    };
}
