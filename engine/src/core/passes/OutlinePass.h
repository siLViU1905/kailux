#pragma once
#include "GraphicsPass.h"
#include "GraphicsPassesPushConstants.h"
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
        
        template<typename... Pcs>
        void push(vk::CommandBuffer cmd, const Pcs &... pcs) const
        {
            pushImpl<kPushConstantRanges, Pcs...>(cmd, pcs...);
        }

    private:
        static constexpr std::string_view kOutlineVertexShaderPath = "shaders/outline_vertex_shader.spv";
        static constexpr std::string_view kOutlineFragmentShaderPath = "shaders/outline_fragment_shader.spv";

        static constexpr std::array kDescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                1, // in id's image
                vk::ShaderStageFlagBits::eFragment
            )
        };
        static constexpr std::array kDescriptorPoolSizes = {
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                1 // id's image
            )
        };
        static_assert(
            check_descriptor_layout_bindings_and_pool_sizes_match(kDescriptorLayoutBindings, kDescriptorPoolSizes),
            "Descriptor layout bindings and pool sizes do not match"
            );

        static constexpr std::array kPushConstantRanges = {
            PushConstantRangeInfo(
                vk::ShaderStageFlagBits::eFragment,
                sizeof(GraphicsPassesPushConstants::Outline)
            )
        };

        static PipelineInfo make_pipeline_info(const Swapchain &swapchain);
    };
}
