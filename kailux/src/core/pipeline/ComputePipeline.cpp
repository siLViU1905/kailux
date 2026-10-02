#include "ComputePipeline.h"

#include "core/Log.h"
#include "core/Shader.h"

namespace kailux
{
    ComputePipeline::ComputePipeline() = default;

    ComputePipeline::ComputePipeline(ComputePipeline &&other) noexcept : PBase(std::move(other))
    {
    }

    ComputePipeline &ComputePipeline::operator=(ComputePipeline &&other) noexcept
    {
        return static_cast<ComputePipeline &>(PBase::operator=(std::move(other)));
    }

    ComputePipeline ComputePipeline::create(const Context &context,
                                            const DescriptorLayout &descriptorSetLayout,
                                            const ComputeShaderInfo &shaderInfo,
                                            std::span<const PushConstantRangeInfo> pushConstantRanges)
    {
        const auto stage{create_compute_shader_module(context, shaderInfo)};
        return make(context, descriptorSetLayout, pushConstantRanges, stage);
    }

    ComputePipeline::ComputePipeline(vk::raii::PipelineLayout &&layout, vk::raii::Pipeline &&pipeline) : PBase(std::move(layout), std::move(pipeline))
    {
    }

    vk::raii::ShaderModule ComputePipeline::create_compute_shader_module(const Context &context, const ComputeShaderInfo& info)
    {
        std::string_view shaderPath = info.computeShaderPath;
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
        return Shader::create_module(context, spirv);
    }

    vk::raii::Pipeline ComputePipeline::create_pipeline_impl(const Context &context,
                                                             const vk::raii::PipelineLayout &layout,
                                                             const vk::raii::ShaderModule &shaderModule)
    {
        const vk::PipelineShaderStageCreateInfo stageInfo(
            {},
            vk::ShaderStageFlagBits::eCompute,
            *shaderModule,
            "main"
        );

        vk::ComputePipelineCreateInfo computeInfo(
            {},
            stageInfo,
            *layout
        );

        return {context.mDevice, nullptr, computeInfo};
    }
}
