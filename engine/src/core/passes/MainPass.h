#pragma once
#include "GraphicsPass.h"

namespace kailux
{
    class MainPass : public GraphicsPass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(MainPass)

        static MainPass create(const Context& context, const Swapchain& swapchain, uint32_t maxFrames);

        void push(vk::CommandBuffer cmd) const override;

        static constexpr uint32_t s_MaxMeshCount = 1'000;

    private:
        static constexpr std::string_view s_VertexShaderPath = "shaders/vertex_shader.spv";
        static constexpr std::string_view s_FragmentShaderPath = "shaders/fragment_shader.spv";

        static constexpr std::array s_DescriptorLayoutBindings = {
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
                s_MaxMeshCount, // albedo
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount, // normal
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount, // roughness
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount, // metallic
                vk::ShaderStageFlagBits::eFragment
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount, // ao
                vk::ShaderStageFlagBits::eFragment
            )
        };
        static constexpr std::array s_DescriptorPoolSizes = {
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
                s_MaxMeshCount // albedo
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount // normal
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount // roughness
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount // metallic
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                s_MaxMeshCount // ao
            )
        };
        static_assert(
            check_descriptor_layout_bindings_and_pool_sizes_match(s_DescriptorLayoutBindings, s_DescriptorPoolSizes),
            "Descriptor layout bindings and pool sizes do not match"
            );

    public:
        static constexpr uint32_t s_MeshTextureBindStart = []() constexpr -> uint32_t {
            for (uint32_t i = 0; i < s_DescriptorLayoutBindings.size(); ++i)
            {
                const auto [descriptor, count, stage] = s_DescriptorLayoutBindings[i];

                if (descriptor == vk::DescriptorType::eCombinedImageSampler &&
                    count == s_MaxMeshCount &&
                    stage == vk::ShaderStageFlagBits::eFragment)
                    return i;
            }
            return ~0u;
        }();
        static_assert(s_MeshTextureBindStart != ~0u, "Failed to find mesh texture bind start index");

    private:
        static PipelineInfo make_pipeline_info(const Swapchain& swapchain, vk::SampleCountFlagBits sampleCount);
    };
}
