#include "Shader.h"

#include <fstream>

#include "Log.h"

namespace kailux
{
    std::string details::read_text_from_file(const std::filesystem::path &path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            log::console.Error("Failed to open shader file {}", path.string());
            return {};
        }

        size_t fileSize = file.tellg();
        std::string buffer(fileSize, '\0');

        file.seekg(0);
        file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
        file.close();

        return buffer;
    }

    std::optional<shaderc_shader_kind> details::to_shaderc_stage_kind(vk::ShaderStageFlagBits stage)
    {
        using S = vk::ShaderStageFlagBits;
        switch (stage)
        {
            case S::eVertex:
                return shaderc_vertex_shader;
                break;
            case S::eFragment:
                return shaderc_fragment_shader;
                break;
            case S::eCompute:
                return shaderc_compute_shader;
                break;
            default:
                log::console.Error("Shader type unknown");
                return std::nullopt;
                break;
        }
    }

    shaderc_optimization_level details::to_shaderc_optimization_kind(ShaderOptimization optimization)
    {
        switch (optimization)
        {
            case ShaderOptimization::None:
                return shaderc_optimization_level_zero;
                break;
            case ShaderOptimization::Space:
                return shaderc_optimization_level_size;
                break;
            case ShaderOptimization::Performance:
                return shaderc_optimization_level_performance;
                break;
            default:
                return shaderc_optimization_level_zero;
                break;
        }
    }

    shaderc::CompileOptions details::make_options(const ShaderCompileInfo &info)
    {
        shaderc::CompileOptions options{};

        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
        options.SetTargetSpirv(shaderc_spirv_version_1_6);

        for (const auto& [name, value] : info.macros)
            options.AddMacroDefinition(name, value);

        if (info.generateDebugInfo)
        {
            options.SetGenerateDebugInfo();
            options.SetOptimizationLevel(shaderc_optimization_level_zero);
        }
        else
            options.SetOptimizationLevel(to_shaderc_optimization_kind(info.optimization));

        options.SetSourceLanguage(info.sourceLanguage == ShaderSourceLanguage::GLSL ? shaderc_source_language_glsl :
                                                                                      shaderc_source_language_hlsl);

        return options;
    }

    std::vector<uint32_t> Shader::compile_from_source(std::string_view source, std::string_view name,
        vk::ShaderStageFlagBits stage, const ShaderCompileInfo &info)
    {
        shaderc::Compiler compiler;
        if (!compiler.IsValid())
        {
            log::console.Error("Shader compiler failed to initialize");
            return {};
        }

        auto shadercStage = details::to_shaderc_stage_kind(stage);
        if (!shadercStage)
            return {};

        auto result = compiler.CompileGlslToSpv(
            source.data(),
            source.size(),
            *shadercStage,
            std::string{name}.c_str(),
            info.entryPoint.c_str(),
            details::make_options(info)
            );

        if (result.GetNumWarnings() > 0)
            log::console.Warning("Shader '{}' warning : {}", name, result.GetErrorMessage());

        if (result.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            log::console.Error("Failed to compile shader '{}':\n{}", name, result.GetErrorMessage());
            return {};
        }
        return {result.cbegin(), result.cend()};
    }

    std::vector<uint32_t> Shader::compile_from_file(const std::filesystem::path &path,
                                                    vk::ShaderStageFlagBits stage, const ShaderCompileInfo &info)
    {
        auto source = details::read_text_from_file(path);
        if (source.empty())
            return {};

        auto spirv = compile_from_source(source, path.string(), stage, info);
        if (!spirv.empty())
            log::console.Debug("shader: compiled '{}' ({} words)", path.string(), spirv.size());

        return spirv;
    }

    void Shader::cache_spirv(const std::filesystem::path &path, std::span<const uint32_t> spirv)
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(spirv.data()), static_cast<std::streamsize>(spirv.size_bytes()));
        log::console.Debug("Cached shader file '{}'", path.filename().string());
    }

    std::vector<uint32_t> Shader::load_spirv(const std::filesystem::path &path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
        {
            log::console.Error("Failed to open shader file {}", path.string());
            return {};
        }

        auto byteSize = static_cast<size_t>(file.tellg());
        if (byteSize == 0 || byteSize % sizeof(uint32_t) != 0)
            {
            log::console.Error("'{}' is not a valid SPIR-V binary", path.string());
            return {};
        }

        std::vector<uint32_t> spirv(byteSize / sizeof(uint32_t));
        file.seekg(0);
        file.read(reinterpret_cast<char *>(spirv.data()), static_cast<std::streamsize>(byteSize));

        return spirv;
    }

    vk::raii::ShaderModule Shader::create_module(const Context &context, std::span<const uint32_t> spirv)
    {
        vk::ShaderModuleCreateInfo info{
            {},
            spirv.size_bytes(),
            spirv.data()
        };

        return {context.mDevice, info};
    }
}
