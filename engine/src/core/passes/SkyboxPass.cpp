#include "SkyboxPass.h"

#include "../descriptor/DescriptorSet.h"
#include "../texture/ImageLoader.h"
#include "../texture/TextureAllocator.h"

namespace kailux
{
    SkyboxPass::SkyboxPass() = default;

    SkyboxPass::SkyboxPass(SkyboxPass &&other) noexcept : GraphicsPass(std::move(other)),
                                                          m_Texture(std::move(other.m_Texture)),
                                                          m_IrradianceMapTexture(std::move(other.m_IrradianceMapTexture)),
                                                          m_PrefilteredEnvTexture(std::move(other.m_PrefilteredEnvTexture)),
                                                          m_BRDFLutTexture(std::move(other.m_BRDFLutTexture))
    {
    }

    SkyboxPass &SkyboxPass::operator=(SkyboxPass &&other) noexcept
    {
        if (this != &other)
        {
            GraphicsPass::operator=(std::move(other));
            m_Texture = std::move(other.m_Texture);
            m_IrradianceMapTexture = std::move(other.m_IrradianceMapTexture);
            m_PrefilteredEnvTexture = std::move(other.m_PrefilteredEnvTexture);
            m_BRDFLutTexture = std::move(other.m_BRDFLutTexture);
        }
        return *this;
    }

    SkyboxPass SkyboxPass::create(const Context &context, const Swapchain &swapchain, uint32_t maxFrames)
    {
        SkyboxPass pass;
        pass.createDescriptorLayout(context, s_DescriptorLayoutBindings);
        pass.createDescriptorPool(context, maxFrames, s_DescriptorPoolSizes);
        pass.createPipeline(
            context,
            swapchain,
            s_VertexShaderPath,
            s_FragmentShaderPath,
            make_pipeline_info(swapchain, context.getMaxUsableSampleCount()),
            {}
            );
        pass.createTexture(context);
        pass.createIrradianceTexture(context);
        pass.createPrefilteredEnvTexture(context);
        pass.createBRDFLutTexture(context);
        return pass;
    }

    void SkyboxPass::push(vk::CommandBuffer cmd) const
    {
        return;
    }

    const Texture &SkyboxPass::getTexture() const
    {
        return m_Texture;
    }

    const Texture & SkyboxPass::getIrradianceMapTexture() const
    {
        return m_IrradianceMapTexture;
    }

    const Texture & SkyboxPass::getPrefilteredEnvTexture() const
    {
        return m_PrefilteredEnvTexture;
    }

    const Texture & SkyboxPass::getBRDFLutTexture() const
    {
        return m_BRDFLutTexture;
    }

    PipelineInfo SkyboxPass::make_pipeline_info(const Swapchain& swapchain, vk::SampleCountFlagBits samples)
    {
        PipelineInfo info;

        info.vertexInputBinding = Vertex::get_binding_description();
        constexpr auto vertexAttribDesc = vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position));
        info.vertexInputAttribute = {vertexAttribDesc};

        info.topology = vk::PrimitiveTopology::eTriangleList;

        info.rasterizer = {
            {},
            vk::False,
            vk::False,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eNone,
            vk::FrontFace::eCounterClockwise,
            vk::False,
            0.f,
            0.f,
            0.f,
            1.f
        };

        vk::PipelineColorBlendAttachmentState colorAttachment;
        colorAttachment.colorWriteMask =
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colorAttachment.blendEnable = vk::False;

        info.colorBlendAttachments.push_back(colorAttachment);
        info.colorFormats.push_back(swapchain.getFormat());

        vk::PipelineColorBlendAttachmentState idAttachment;
        idAttachment.blendEnable = vk::False;
        idAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR;

        info.colorBlendAttachments.push_back(idAttachment);
        info.colorFormats.push_back(vk::Format::eR32Uint);

        info.samples = samples;

        info.depthStencilInfo.depthTestEnable = vk::True;
        info.depthStencilInfo.depthWriteEnable = vk::False;
        info.depthStencilInfo.depthCompareOp = vk::CompareOp::eLessOrEqual;

        info.depthStencilInfo.depthBoundsTestEnable = vk::False;
        info.depthStencilInfo.stencilTestEnable = vk::False;

        return info;
    }

    void SkyboxPass::createTexture(const Context &context)
    {
        std::array<ImageLoader::ImageData, 6> faces;
        int i = 0;
        for (auto path: s_SkyboxTexturePaths)
        {
            auto result = ImageLoader::load_image(path);
            if (!result)
                return;
            faces[i++] = *result;
        }

        m_Texture = TextureAllocator::create_cubemap(context, faces);
    }

    void SkyboxPass::createIrradianceTexture(const Context &context)
    {
        std::array<ImageLoader::ImageData, 6> faces;
        int i = 0;
        for (auto path: s_IrradianceTexturePaths)
        {
            auto result = ImageLoader::load_image(path);
            if (!result)
                return;
            faces[i++] = *result;
        }

        m_IrradianceMapTexture = TextureAllocator::create_cubemap(context, faces);
    }

    void SkyboxPass::createPrefilteredEnvTexture(const Context &context)
    {
        static constexpr std::array<std::string_view, 6> faceNames = {
            "px", "nx", "py", "ny", "pz", "nz"
        };

        std::vector<std::array<ImageLoader::ImageData, 6>> mips(s_PrefilteredMipLevels);

        for (uint32_t mip = 0; mip < s_PrefilteredMipLevels; mip++)
            for (uint32_t face = 0; face < 6; face++)
            {
                auto path = std::string(s_PrefilteredBasePath)
                                 + std::to_string(mip)
                                 + "_"
                                 + std::string(faceNames[face])
                                 + ".png";

                auto result = ImageLoader::load_image(path);
                if (!result)
                    return;

                mips[mip][face] = std::move(*result);
            }

        m_PrefilteredEnvTexture = TextureAllocator::create_cubemap_with_mips(context, mips);
    }

    void SkyboxPass::createBRDFLutTexture(const Context &context)
    {
        if (auto data = ImageLoader::load_image(s_BRDFLutPath))
            m_BRDFLutTexture = TextureAllocator::create_from_image_data(context, *data);
    }
}
