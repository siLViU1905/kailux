#include "Pipeline.h"

#include <filesystem>
#include <fstream>

#include "descriptor/DescriptorLayout.h"
#include "Log.h"
#include "mesh/Vertex.h"

namespace kailux
{
    Pipeline::Pipeline() : m_Layout({}), m_Pipeline({})
    {
    }

    Pipeline::Pipeline(Pipeline &&other) noexcept : m_Layout(std::move(other.m_Layout)),
                                                    m_Pipeline(std::move(other.m_Pipeline))
    {
    }

    Pipeline &Pipeline::operator=(Pipeline &&other) noexcept
    {
        if (this != &other)
        {
            m_Layout = std::move(other.m_Layout);
            m_Pipeline = std::move(other.m_Pipeline);
        }
        return *this;
    }

    Pipeline Pipeline::createGraphics(const Context &context, const Swapchain &swapchain,
                                      const DescriptorLayout &descriptorSetLayout, const GraphicsShaderInfo &shaderInfo,
                                      const PipelineInfo &pipelineInfo,
                                      std::span<const PushConstantRangeInfo> pushConstantRanges)
    {
        KAILUX_LOG_PARENT_CLR_YELLOW("[Graphics Pipeline]")
        Pipeline pipeline;

        auto shaderModules = create_graphics_shader_modules(context, shaderInfo);
        KAILUX_LOG_CHILD_CLR_YELLOW("Created shader modules")

        pipeline.createLayout(context, descriptorSetLayout, pushConstantRanges);
        KAILUX_LOG_CHILD_CLR_YELLOW("Created pipeline layout")

        pipeline.createGraphicsPipeline(context, swapchain, shaderModules, pipelineInfo);
        KAILUX_LOG_CHILD_CLR_YELLOW("Created graphics pipeline")

        return pipeline;
    }

    Pipeline Pipeline::createCompute(const Context &context, const DescriptorLayout &descriptorSetLayout,
                                     const ComputeShaderInfo &shaderInfo,
                                     std::span<const PushConstantRangeInfo> pushConstantRanges)
    {
        KAILUX_LOG_PARENT_CLR_YELLOW("[Compute Pipeline]")
        Pipeline pipeline;

        auto shaderModule = create_shader_module(context, read_shader_from_file(shaderInfo.computeShaderPath));
        KAILUX_LOG_CHILD_CLR_YELLOW("Created shader module")

        pipeline.createLayout(context, descriptorSetLayout, pushConstantRanges);
        KAILUX_LOG_CHILD_CLR_YELLOW("Created pipeline layout")

        pipeline.createComputePipeline(context, shaderModule);
        KAILUX_LOG_CHILD_CLR_YELLOW("Created compute pipeline")

        return pipeline;
    }

    void Pipeline::bindGraphics(vk::CommandBuffer cmd) const
    {
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);
    }

    void Pipeline::bindCompute(vk::CommandBuffer cmd) const
    {
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute, m_Pipeline);
    }

    vk::PipelineLayout Pipeline::getLayout() const
    {
        return *m_Layout;
    }

    std::vector<vk::PipelineShaderStageCreateInfo> Pipeline::ShaderModules::makeVkStages() const
    {
        std::vector<vk::PipelineShaderStageCreateInfo> stages;
        stages.reserve(modules.size());

        for (const auto &instance: modules)
        {
            stages.emplace_back(
                vk::PipelineShaderStageCreateFlags{},
                instance.stage,
                *instance.module,
                "main"
            );
        }
        return stages;
    }

    std::vector<char> Pipeline::read_shader_from_file(std::string_view path)
    {
        std::ifstream file(std::string(path), std::ios::ate | std::ios::binary);

        if (!file.is_open())
            throw std::runtime_error(std::format("Failed to open shader with path {}", path));

        size_t fileSize = file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    vk::raii::ShaderModule Pipeline::create_shader_module(const Context &context, const std::vector<char> &code)
    {
        vk::ShaderModuleCreateInfo createInfo{};

        createInfo.codeSize = code.size() * sizeof(char);
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        return {context.m_Device, createInfo};
    }

    Pipeline::ShaderModules Pipeline::create_graphics_shader_modules(const Context &context,
                                                                     const GraphicsShaderInfo &stages)
    {
        ShaderModules result;
        result.modules.reserve(stages.size());

        for (const auto &[stage, path]: stages)
        {
            auto code = read_shader_from_file(path);
            auto module = create_shader_module(context, code);

            result.modules.emplace_back(stage, std::move(module));
        }

        return result;
    }

    void Pipeline::createLayout(const Context &context, const DescriptorLayout &descriptorLayout,
                                std::span<const PushConstantRangeInfo> pushConstantRanges)
    {
        const auto dsLayout = descriptorLayout.getLayout();

        std::vector<vk::PushConstantRange> ranges;
        uint32_t offset = 0;
        for (auto range: pushConstantRanges)
        {
            ranges.emplace_back(
                range.shaderStage,
                offset,
                range.size
            );
            offset += range.size;
        }

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
            {},
            1,
            &dsLayout,
            static_cast<uint32_t>(ranges.size()),
            ranges.data()
        );

        m_Layout = vk::raii::PipelineLayout(context.m_Device, pipelineLayoutInfo);
    }

    void Pipeline::createGraphicsPipeline(const Context &context, const Swapchain &swapchain,
                                          const ShaderModules &shaderModules, const PipelineInfo &info)
    {
        constexpr std::array dynamicStates =
        {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };

        vk::PipelineDynamicStateCreateInfo dynamicState(
            {},
            static_cast<uint32_t>(dynamicStates.size()),
            dynamicStates.data()
        );

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
            {},
            1,
            &info.vertexInputBinding,
            static_cast<uint32_t>(info.vertexInputAttribute.size()),
            info.vertexInputAttribute.data()
        );

        vk::PipelineViewportStateCreateInfo viewportState({}, 1, {}, 1);
        vk::PipelineColorBlendStateCreateInfo colorBlending(
            {},
            vk::False,
            vk::LogicOp::eCopy,
            static_cast<uint32_t>(info.colorBlendAttachments.size()),
            info.colorBlendAttachments.data()
        );

        vk::PipelineMultisampleStateCreateInfo multisampling(
            {},
            info.samples,
            vk::False,
            1.f
        );

        vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo(
            {},
            static_cast<uint32_t>(info.colorFormats.size()),
            info.colorFormats.data(),
            swapchain.getDepthFormat()
        );

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
            {},
            info.topology
        );

        auto shaderStages = shaderModules.makeVkStages();

        vk::GraphicsPipelineCreateInfo pipelineInfo(
            {},
            static_cast<uint32_t>(shaderStages.size()),
            shaderStages.data(),
            &vertexInputInfo,
            &inputAssembly,
            {},
            &viewportState,
            &info.rasterizer,
            &multisampling,
            &info.depthStencilInfo,
            &colorBlending,
            &dynamicState,
            *m_Layout,
            {}
        );
        pipelineInfo.pNext = &pipelineRenderingCreateInfo;

        m_Pipeline = vk::raii::Pipeline(context.m_Device, nullptr, pipelineInfo);
    }

    void Pipeline::createComputePipeline(const Context &context, const vk::raii::ShaderModule &shaderModule)
    {
        vk::PipelineShaderStageCreateInfo stageInfo(
            {},
            vk::ShaderStageFlagBits::eCompute,
            *shaderModule,
            "main"
        );

        vk::ComputePipelineCreateInfo computeInfo(
            {},
            stageInfo,
            *m_Layout
        );

        m_Pipeline = vk::raii::Pipeline(context.m_Device, nullptr, computeInfo);
    };
}
