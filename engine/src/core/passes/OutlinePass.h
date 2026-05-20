#pragma once
#include "GraphicsPass.h"
#include "core/Core.h"
#include "core/Pipeline.h"
#include "core/descriptor/DescriptorPool.h"

namespace kailux
{
    class OutlinePass : public GraphicsPass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(OutlinePass)

        static OutlinePass create(const Context& context, const Swapchain& swapchain, uint32_t frameCount);
        
        void push(vk::CommandBuffer cmd) const override;

        void setColorAndId(glm::vec3 color, uint32_t id);

    private:
        static constexpr std::string_view s_OutlineVertexShaderPath = "shaders/outline_vertex_shader.spv";
        static constexpr std::string_view s_OutlineFragmentShaderPath = "shaders/outline_fragment_shader.spv";

        static constexpr std::array s_DescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                1, // in id's image
                vk::ShaderStageFlagBits::eFragment
            )
        };
        static constexpr std::array s_DescriptorPoolSizes = {
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                1 // id's image
            )
        };
        static_assert(
            check_descriptor_layout_bindings_and_pool_sizes_match(s_DescriptorLayoutBindings, s_DescriptorPoolSizes),
            "Descriptor layout bindings and pool sizes do not match"
            );

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

        OutlinePushConstant m_Pc;
    };
}
