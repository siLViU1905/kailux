#include "Pipeline.h"

#include <filesystem>
#include <fstream>

#include "DescriptorSetLayout.h"
#include "Logger.h"
#include "Swapchain.h"
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

    Pipeline Pipeline::create(const Context &context, const Swapchain &swapchain,
                              const DescriptorSetLayout &descriptorSetLayout, const ShaderInfo &shaderInfo,
                              const PipelineInfo &pipelineInfo)
    {
        KAILUX_LOG_PARENT_CLR_YELLOW("[Pipeline]")
        Pipeline pipeline;

        auto shaderModules = create_shader_modules(context, shaderInfo);
        KAILUX_LOG_CHILD_CLR_YELLOW("Created shader modules")

        pipeline.createLayout(context, descriptorSetLayout);
        KAILUX_LOG_CHILD_CLR_YELLOW("Created pipeline layout")

        pipeline.createPipeline(context, swapchain, shaderModules, pipelineInfo);
        KAILUX_LOG_CHILD_CLR_YELLOW("Created pipeline")

        return pipeline;
    }

    std::array<vk::PipelineShaderStageCreateInfo, 2> Pipeline::ShaderModules::getStages() const
    {
        return {
            vk::PipelineShaderStageCreateInfo(
                {},
                vk::ShaderStageFlagBits::eVertex,
                vertex,
                "main"
            ),
            vk::PipelineShaderStageCreateInfo(
                {},
                vk::ShaderStageFlagBits::eFragment,
                fragment,
                "main"
            )
        };
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

    Pipeline::ShaderModules Pipeline::create_shader_modules(const Context &context, const ShaderInfo &info)
    {
        auto vsModule = create_shader_module(context, read_shader_from_file(info.vertexShaderPath));
        auto fsModule = create_shader_module(context, read_shader_from_file(info.fragmentShaderPath));

        return {std::move(vsModule), std::move(fsModule)};
    }

    void Pipeline::createLayout(const Context &context, const DescriptorSetLayout &descriptorSetLayout)
    {
        const auto dsLayout = descriptorSetLayout.getLayout();

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
            {},
            1,
            &dsLayout
        );

        m_Layout = vk::raii::PipelineLayout(context.m_Device, pipelineLayoutInfo);
    }

    void Pipeline::createPipeline(const Context &context, const Swapchain &swapchain,
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

        constexpr auto bindingDescription = Vertex::get_binding_description();
        constexpr auto attributeDescription = Vertex::get_attribute_description();

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
            {},
            1,
            &bindingDescription,
            static_cast<uint32_t>(attributeDescription.size()),
            attributeDescription.data()
        );

        auto viewport = vk::Viewport(0.f, 0.f,
                                     static_cast<float>(swapchain.getExtent().width),
                                     static_cast<float>(swapchain.getExtent().height),
                                     0.f, 1.f);

        auto scissors = vk::Rect2D({0, 0}, swapchain.getExtent());

        vk::PipelineViewportStateCreateInfo viewportState({}, 1, {}, 1);
        vk::PipelineColorBlendStateCreateInfo colorBlending(
            {},
            vk::False,
            vk::LogicOp::eCopy,
            1,
            &info.colorBlendAttachment
        );

        vk::PipelineMultisampleStateCreateInfo multisampling(
            {},
            info.samples,
            vk::False,
            1.f
        );

        auto shaderStages = shaderModules.getStages();

        auto format = swapchain.getFormat();
        vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo(
            {},
            1,
            &format,
            swapchain.getDepthFormat()
        );

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
            {},
            info.topology
        );

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
    };
}
