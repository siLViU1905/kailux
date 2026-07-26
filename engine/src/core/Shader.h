#pragma once
#include "Context.h"
#include <shaderc/shaderc.hpp>

namespace kailux
{
    enum class ShaderOptimization : uint8_t
    {
        None,
        Space,
        Performance
    };

    enum class ShaderSourceLanguage : uint8_t
    {
        GLSL,
        HLSL
    };

    struct ShaderMacro
    {
        std::string name;
        std::string value;
    };

    struct ShaderCompileInfo;

    namespace details
    {
        consteval ShaderOptimization get_compiled_shader_optimization()
        {
            if constexpr (kailux::details::kCompiledLevel == kailux::details::CompileLevel::Debug)
                return ShaderOptimization::None;
            else if constexpr (kailux::details::kCompiledLevel == kailux::details::CompileLevel::Release)
                return ShaderOptimization::Performance;
            return ShaderOptimization::None;
        }
        std::string read_text_from_file(const std::filesystem::path& path);

        std::optional<shaderc_shader_kind> to_shaderc_stage_kind(vk::ShaderStageFlagBits stage);
        shaderc_optimization_level         to_shaderc_optimization_kind(ShaderOptimization optimization);

        shaderc::CompileOptions make_options(const ShaderCompileInfo& info);
    }

    struct ShaderCompileInfo
    {
        std::vector<std::filesystem::path> includeDirs;
        std::vector<ShaderMacro>           macros;
        std::string                        entryPoint{"main"};
        ShaderOptimization                 optimization{details::get_compiled_shader_optimization()};
        ShaderSourceLanguage               sourceLanguage{ShaderSourceLanguage::GLSL};
        bool                               generateDebugInfo{false};
    };

    class Shader
    {
    public:
        static std::vector<uint32_t> compile_from_source(std::string_view source, std::string_view name,
                                                         vk::ShaderStageFlagBits stage,
                                                         const ShaderCompileInfo &info = {});

        static std::vector<uint32_t> compile_from_file(const std::filesystem::path &path, vk::ShaderStageFlagBits stage,
                                                       const ShaderCompileInfo &info = {});

        static std::vector<uint32_t> load_spirv(const std::filesystem::path &path);

        static vk::raii::ShaderModule create_module(const Context &context, std::span<const uint32_t> spirv);
    };
}
