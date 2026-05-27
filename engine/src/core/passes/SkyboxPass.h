#pragma once
#include "GraphicsPass.h"
#include "../descriptor/DescriptorLayout.h"
#include "../Pipeline.h"
#include "../descriptor/DescriptorPool.h"
#include "../descriptor/DescriptorSet.h"
#include "../mesh/MeshRegistry.h"
#include "../texture/Texture.h"

namespace kailux
{
    class SkyboxPass : public GraphicsPass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(SkyboxPass)

        static SkyboxPass create(const Context& context, const Swapchain& swapchain, uint32_t maxFrames);

        template<typename... Pcs>
        void push(vk::CommandBuffer cmd, const Pcs &... pcs) const
        {
            pushImpl<{}, Pcs...>(cmd, pcs...);
        }

        const Texture&          getTexture() const;
        const Texture&          getIrradianceMapTexture() const;
        const Texture&          getPrefilteredEnvTexture() const;
        const Texture&          getBRDFLutTexture() const;

    private:
        static constexpr std::string_view s_VertexShaderPath = "shaders/skybox_vertex_shader.spv";
        static constexpr std::string_view s_FragmentShaderPath = "shaders/skybox_fragment_shader.spv";

        static constexpr std::array<std::string_view, 6> s_SkyboxTexturePaths = {
            "assets/cubemap/px.png",
            "assets/cubemap/nx.png",
            "assets/cubemap/py.png",
            "assets/cubemap/ny.png",
            "assets/cubemap/pz.png",
            "assets/cubemap/nz.png"
        };
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
        static_assert(
            check_descriptor_layout_bindings_and_pool_sizes_match(s_DescriptorLayoutBindings, s_DescriptorPoolSizes),
            "Descriptor layout bindings and pool sizes do not match"
            );

        static PipelineInfo make_pipeline_info(const Swapchain& swapchain, vk::SampleCountFlagBits samples);

        void createTexture(const Context &context);
        void createIrradianceTexture(const Context& context);
        void createPrefilteredEnvTexture(const Context& context);
        void createBRDFLutTexture(const Context& context);

        Texture          m_Texture;
        Texture          m_IrradianceMapTexture;
        Texture          m_PrefilteredEnvTexture;
        Texture          m_BRDFLutTexture;
    };
}
