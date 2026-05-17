#pragma once
#include "core/Core.h"
#include "core/Pipeline.h"
#include "core/descriptor/DescriptorPool.h"

namespace kailux
{
    class OutlinePass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(OutlinePass)

        static OutlinePass create(const Context& context, const Swapchain& swapchain, uint32_t frameCount, std::string_view vertShaderPath, std::string_view fragShaderPath);

        void bind(vk::CommandBuffer cmd) const;
        
        void push(vk::CommandBuffer cmd) const;

        const DescriptorLayout& getDescriptorLayout() const;
        const DescriptorPool&   getDescriptorPool() const;
        const Pipeline&         getPipeline() const;

        void setColorAndId(glm::vec3 color, uint32_t id);

    private:
        static constexpr std::array s_DescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                1, // in id's image
                vk::ShaderStageFlagBits::eFragment
            )
        };
        static constexpr std::array s_DescriptorLayoutSizes = {
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                1 // id's image
            )
        };

        struct OutlinePushConstant
        {
            glm::vec4               color{};
            uint32_t                id = ~0u;
            std::array<uint32_t, 3> _padding;
        };

        KAILUX_CHECK_DATA_STRUCTURE_SIZE(OutlinePushConstant)

        static constexpr std::array s_PushConstantRanges = {
            PushConstantRangeInfo(
                vk::ShaderStageFlagBits::eFragment,
                sizeof(OutlinePushConstant)
            )
        };

        static PipelineInfo make_pipeline_info(const Swapchain &swapchain);

        void createDescriptorLayout(const Context &context);
        void createDescriptorPool(const Context &context, uint32_t frameCount);
        void createPipeline(const Context &context, const Swapchain &swapchain, std::string_view vertShaderPath, std::string_view fragShaderPath);

        DescriptorLayout    m_DescriptorLayout;
        DescriptorPool      m_DescriptorPool;
        Pipeline            m_Pipeline;

        OutlinePushConstant m_Pc;
    };
}
