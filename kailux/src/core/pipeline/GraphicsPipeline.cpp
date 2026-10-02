#include "GraphicsPipeline.h"

#include "core/Log.h"
#include "core/Shader.h"

namespace kailux
{
    GraphicsPipeline::GraphicsPipeline() = default;

    GraphicsPipeline::GraphicsPipeline(GraphicsPipeline &&other) noexcept : PBase(std::move(other))
    {
    }

    GraphicsPipeline &GraphicsPipeline::operator=(GraphicsPipeline &&other) noexcept
    {
        return static_cast<GraphicsPipeline &>(PBase::operator=(std::move(other)));
    }

    GraphicsPipeline GraphicsPipeline::create(const Context &context, const Swapchain &swapchain,
        const DescriptorLayout &descriptorSetLayout, const GraphicsShaderInfo &shaderInfo,
        const PipelineInfo &pipelineInfo, std::span<const PushConstantRangeInfo> pushConstantRanges)
    {
        const auto stages{create_graphics_shader_modules(context, shaderInfo)};
        return make(context, descriptorSetLayout, pushConstantRanges, swapchain, stages, pipelineInfo);
    }

    GraphicsPipeline::GraphicsPipeline(vk::raii::PipelineLayout &&layout, vk::raii::Pipeline &&pipeline) : PBase(std::move(layout), std::move(pipeline))
    {
    }

    Pipeline<GraphicsPipeline>::ShaderModules GraphicsPipeline::create_graphics_shader_modules(const Context &context,
                                                                                               const GraphicsShaderInfo &stages)
    {
        ShaderModules result;
        result.modules.reserve(stages.size());

        for (const auto &[stage, path]: stages)
        {
            auto cacheFile = path.substr(0, path.find_last_of('.'));
            cacheFile += ".spv";
            std::vector<uint32_t> spirv;
            if (std::filesystem::exists(cacheFile))
            {
                spirv = Shader::load_spirv(cacheFile);
                log::console.Debug("Found cached spirv '{}'", cacheFile);
            }
            else
            {
                spirv = Shader::compile_from_file(path, stage);
                Shader::cache_spirv(cacheFile, spirv);
            }
            auto module = Shader::create_module(context, spirv);

            result.modules.emplace_back(stage, std::move(module));
        }

        return result;
    }

    vk::raii::Pipeline GraphicsPipeline::create_pipeline_impl(const Context &context,
                                                              const vk::raii::PipelineLayout &layout, const Swapchain &swapchain, const ShaderModules &shaderModules,
                                                              const PipelineInfo &info)
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

        constexpr vk::PipelineViewportStateCreateInfo viewportState({}, 1, {}, 1);
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
            swapchain.GetDepthFormat()
        );

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
            {},
            info.topology
        );

        auto shaderStages = shaderModules.MakeVkStages();

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
            *layout,
            {}
        );
        pipelineInfo.pNext = &pipelineRenderingCreateInfo;

        return {context.mDevice, nullptr, pipelineInfo};
    }
}
