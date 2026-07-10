#pragma once
#include "GraphicsPass.h"

namespace kailux
{
    class MainPass : public GraphicsPass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(MainPass)

        static MainPass create(const Context& context, const Swapchain& swapchain, uint32_t maxFrames);

        template<typename... Pcs>
        void push(vk::CommandBuffer cmd, const Pcs &... pcs) const
        {
            pushImpl<{}, Pcs...>(cmd, pcs...);
        }

    private:
        static constexpr std::string_view kVertexShaderPath = "shaders/vertex_shader.spv";
        static constexpr std::string_view kFragmentShaderPath = "shaders/fragment_shader.spv";

        static constexpr std::array kDescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eUniformBuffer,
                1, // camera
                vk::ShaderStageFlagBits::eVertex
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // model
                vk::ShaderStageFlagBits::eVertex
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // material
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // scene
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                1, // skybox
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                1, // irradiance map
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                1, // prefiltered env
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                1, // brdf lut
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                details::kMaxTextures, // textures array
                vk::ShaderStageFlagBits::eFragment
            )
        };
        static constexpr std::array kDescriptorPoolSizes = {
            DescriptorPoolSize(
                vk::DescriptorType::eUniformBuffer,
                1 // camera
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // model
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // material
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // scene
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                1 // skybox
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                1 // irradiance map
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                1 // prefiltered env
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                1 // brdf lut
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                details::kMaxTextures // textures array
            )
        };
        static_assert(
            check_descriptor_layout_bindings_and_pool_sizes_match(kDescriptorLayoutBindings, kDescriptorPoolSizes),
            "Descriptor layout bindings and pool sizes do not match"
            );

    public:
        static constexpr uint32_t kMeshTextureBindStart = []() constexpr -> uint32_t {
            for (uint32_t i = 0; i < kDescriptorLayoutBindings.size(); ++i)
            {
                const auto [descriptor, count, stage] = kDescriptorLayoutBindings[i];

                if (descriptor == vk::DescriptorType::eCombinedImageSampler &&
                    count == details::kMaxTextures &&
                    stage == vk::ShaderStageFlagBits::eFragment)
                    return i;
            }
            return ~0u;
        }();
        static_assert(kMeshTextureBindStart != ~0u, "Failed to find mesh texture bind start index");

    private:
        static PipelineInfo make_pipeline_info(const Swapchain& swapchain, vk::SampleCountFlagBits sampleCount);
    };
}
