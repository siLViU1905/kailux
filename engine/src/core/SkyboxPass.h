#pragma once
#include "descriptor/DescriptorLayout.h"
#include "Pipeline.h"
#include "descriptor/DescriptorPool.h"
#include "descriptor/DescriptorSet.h"
#include "mesh/MeshRegistry.h"
#include "texture/Texture.h"

namespace kailux
{
    class SkyboxPass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(SkyboxPass)

        static SkyboxPass create(const Context& context, const Swapchain& swapchain, uint32_t sets, const std::array<std::string_view, 6> &paths);

        const Texture&          getTexture() const;
        const Texture&          getIrradianceMapTexture() const;
        const Texture&          getPrefilteredEnvTexture() const;
        const Texture&          getBRDFLutTexture() const;
        const DescriptorLayout& getDescriptorLayout() const;
        const DescriptorPool&   getDescriptorPool() const;

        void render(vk::CommandBuffer cmd, const DescriptorSet& descriptorSet, MeshView cubeView) const;

    private:
        static constexpr std::string_view s_VertexShaderPath = "shaders/skybox_vertex_shader.spv";
        static constexpr std::string_view s_FragmentShaderPath = "shaders/skybox_fragment_shader.spv";

        static constexpr std::array<std::string_view, 6> s_IrradianceTexturePaths = {
            "assets/ibl/irradiance/i_px.png",
            "assets/ibl/irradiance/i_nx.png",
            "assets/ibl/irradiance/i_py.png",
            "assets/ibl/irradiance/i_ny.png",
            "assets/ibl/irradiance/i_pz.png",
            "assets/ibl/irradiance/i_nz.png"
        };
        static constexpr std::string_view s_PrefilteredBasePath = "assets/ibl/prefiltered/m";
        static constexpr std::string_view s_BRDFLutPath = "assets/ibl/brdf_lut.png";

        static constexpr uint32_t         s_PrefilteredMipLevels = 5;

        static constexpr std::array s_DescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eUniformBuffer,
                1, // camera
                vk::ShaderStageFlagBits::eVertex
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eCombinedImageSampler,
                1, // cube sampler
                vk::ShaderStageFlagBits::eFragment
            )
        };
        static constexpr std::array s_DescriptorPoolSizes = {
            DescriptorPoolSize(
                vk::DescriptorType::eUniformBuffer,
                1 // camera
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eCombinedImageSampler,
                1 // cube sampler
            )
        };

        static PipelineInfo make_pipeline_info(vk::SampleCountFlagBits samples);

        void createDescriptorResources(const Context& context, uint32_t sets);
        void createPipeline(const Context& context, const Swapchain& swapchain);
        void createTexture(const Context& context, const std::array<std::string_view, 6> &paths);
        void createIrradianceTexture(const Context& context);
        void createPrefilteredEnvTexture(const Context& context);
        void createBRDFLutTexture(const Context& context);

        DescriptorLayout m_DescriptorLayout;
        Pipeline         m_Pipeline;
        DescriptorPool   m_DescriptorPool;
        Texture          m_Texture;
        Texture          m_IrradianceMapTexture;
        Texture          m_PrefilteredEnvTexture;
        Texture          m_BRDFLutTexture;
    };
}
