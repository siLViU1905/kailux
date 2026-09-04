#include "Pipeline.h"

#include <filesystem>
#include <fstream>

#include "descriptor/DescriptorLayout.h"
#include "Log.h"
#include "Shader.h"
#include "mesh/Vertex.h"

namespace kailux
{
    Pipeline::Pipeline() : mLayout({}), mPipeline({})
    {
    }

    Pipeline::Pipeline(Pipeline &&other) noexcept : mLayout(std::move(other.mLayout)),
                                                    mPipeline(std::move(other.mPipeline))
    {
    }

    Pipeline &Pipeline::operator=(Pipeline &&other) noexcept
    {
        if (this != &other)
        {
            mLayout = std::move(other.mLayout);
            mPipeline = std::move(other.mPipeline);
        }
        return *this;
    }

    Pipeline Pipeline::create_graphics(const Context &context, const Swapchain &swapchain,
                                      const DescriptorLayout &descriptorSetLayout, const GraphicsShaderInfo &shaderInfo,
                                      const PipelineInfo &pipelineInfo,
                                      std::span<const PushConstantRangeInfo> pushConstantRanges)
    {
        Pipeline pipeline;

        auto shaderModules = create_graphics_shader_modules(context, shaderInfo);
        log::console.Debug("graphics pipeline: module shader created");

        pipeline.CreateLayout(context, descriptorSetLayout, pushConstantRanges);
        log::console.Debug("graphics pipeline: layout created");

        pipeline.CreateGraphicsPipeline(context, swapchain, shaderModules, pipelineInfo);
        log::console.Debug("graphics pipeline: pipeline created");

        return pipeline;
    }

    Pipeline Pipeline::create_compute(const Context &context, const DescriptorLayout &descriptorSetLayout,
                                     const ComputeShaderInfo &shaderInfo,
                                     std::span<const PushConstantRangeInfo> pushConstantRanges)
    {
        log::console.Debug("compute pipeline: creating");
        Pipeline pipeline;

        std::string_view shaderPath = shaderInfo.computeShaderPath;
        std::string cacheFile{shaderPath.substr(0, shaderPath.find_last_of('.'))};
        cacheFile += ".spv";
        std::vector<uint32_t> spirv;
        if (std::filesystem::exists(cacheFile))
        {
            spirv = Shader::load_spirv(cacheFile);
            log::console.Debug("Found cached spirv '{}'", cacheFile);
        }
        else
        {
            spirv = Shader::compile_from_file(shaderPath, vk::ShaderStageFlagBits::eCompute);
            Shader::cache_spirv(cacheFile, spirv);
        }
        auto shaderModule = Shader::create_module(context, spirv);
        log::console.Debug("compute pipeline: shader module created");

        pipeline.CreateLayout(context, descriptorSetLayout, pushConstantRanges);
        log::console.Debug("compute pipeline: layout created");

        pipeline.CreateComputePipeline(context, shaderModule);
        log::console.Debug("compute pipeline: pipeline created");

        return pipeline;
    }

    void Pipeline::BindGraphics(vk::CommandBuffer cmd) const
    {
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline);
    }

    void Pipeline::BindCompute(vk::CommandBuffer cmd) const
    {
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute, mPipeline);
    }

    vk::PipelineLayout Pipeline::GetLayout() const
    {
        return *mLayout;
    }

    std::vector<vk::PipelineShaderStageCreateInfo> Pipeline::ShaderModules::MakeVkStages() const
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

    Pipeline::ShaderModules Pipeline::create_graphics_shader_modules(const Context &context,
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

    void Pipeline::CreateLayout(const Context &context, const DescriptorLayout &descriptorLayout,
                                std::span<const PushConstantRangeInfo> pushConstantRanges)
    {
        const auto dsLayout = descriptorLayout.GetLayout();

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

        mLayout = vk::raii::PipelineLayout(context.mDevice, pipelineLayoutInfo);
    }

    void Pipeline::CreateGraphicsPipeline(const Context &context, const Swapchain &swapchain,
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
            *mLayout,
            {}
        );
        pipelineInfo.pNext = &pipelineRenderingCreateInfo;

        mPipeline = vk::raii::Pipeline(context.mDevice, nullptr, pipelineInfo);
    }

    void Pipeline::CreateComputePipeline(const Context &context, const vk::raii::ShaderModule &shaderModule)
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
            *mLayout
        );

        mPipeline = vk::raii::Pipeline(context.mDevice, nullptr, computeInfo);
    };
}
