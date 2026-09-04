#include "SkyboxPass.h"

#include "../descriptor/DescriptorSet.h"
#include "../texture/ImageLoader.h"
#include "../texture/TextureAllocator.h"

namespace kailux
{
    SkyboxPass::SkyboxPass() = default;

    SkyboxPass::SkyboxPass(SkyboxPass &&other) noexcept : GraphicsPass(std::move(other)),
                                                          mTexture(std::move(other.mTexture)),
                                                          mIrradianceMapTexture(std::move(other.mIrradianceMapTexture)),
                                                          mPrefilteredEnvTexture(std::move(other.mPrefilteredEnvTexture)),
                                                          mBRDFLutTexture(std::move(other.mBRDFLutTexture))
    {
    }

    SkyboxPass &SkyboxPass::operator=(SkyboxPass &&other) noexcept
    {
        if (this != &other)
        {
            GraphicsPass::operator=(std::move(other));
            mTexture = std::move(other.mTexture);
            mIrradianceMapTexture = std::move(other.mIrradianceMapTexture);
            mPrefilteredEnvTexture = std::move(other.mPrefilteredEnvTexture);
            mBRDFLutTexture = std::move(other.mBRDFLutTexture);
        }
        return *this;
    }

    SkyboxPass SkyboxPass::create(const Context &context, const Swapchain &swapchain, uint32_t maxFrames)
    {
        SkyboxPass pass;
        pass.CreateDescriptorLayout(context, kDescriptorLayoutBindings);
        pass.CreateDescriptorPool(context, maxFrames, kDescriptorPoolSizes);
        pass.CreatePipeline(
            context,
            swapchain,
            kVertexShaderPath,
            kFragmentShaderPath,
            make_pipeline_info(swapchain, context.GetMaxUsableSampleCount()),
            kPushConstantRanges
        );
        pass.CreateTexture(context);
        pass.CreateIrradianceTexture(context);
        pass.CreatePrefilteredEnvTexture(context);
        pass.CreateBrdfLutTexture(context);
        return pass;
    }

    const Texture &SkyboxPass::GetTexture() const
    {
        return mTexture;
    }

    const Texture & SkyboxPass::GetIrradianceMapTexture() const
    {
        return mIrradianceMapTexture;
    }

    const Texture & SkyboxPass::GetPrefilteredEnvTexture() const
    {
        return mPrefilteredEnvTexture;
    }

    const Texture & SkyboxPass::GetBrdfLutTexture() const
    {
        return mBRDFLutTexture;
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
        info.colorFormats.push_back(swapchain.GetFormat());

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

    void SkyboxPass::CreateTexture(const Context &context)
    {
        std::array<ImageLoader::ImageData, 6> faces;
        int i = 0;
        for (auto path: kSkyboxTexturePaths)
        {
            auto result = ImageLoader::load_image(path);
            if (!result)
                return;
            faces[i++] = *result;
        }

        mTexture = TextureAllocator::create_cubemap(context, faces);
    }

    void SkyboxPass::CreateIrradianceTexture(const Context &context)
    {
        std::array<ImageLoader::ImageData, 6> faces;
        int i = 0;
        for (auto path: kIrradianceTexturePaths)
        {
            auto result = ImageLoader::load_image(path);
            if (!result)
                return;
            faces[i++] = *result;
        }

        mIrradianceMapTexture = TextureAllocator::create_cubemap(context, faces);
    }

    void SkyboxPass::CreatePrefilteredEnvTexture(const Context &context)
    {
        static constexpr std::array<std::string_view, 6> faceNames = {
            "px", "nx", "py", "ny", "pz", "nz"
        };

        std::vector<std::array<ImageLoader::ImageData, 6>> mips(kPrefilteredMipLevels);

        for (uint32_t mip = 0; mip < kPrefilteredMipLevels; mip++)
            for (uint32_t face = 0; face < 6; face++)
            {
                auto path = std::string(kPrefilteredBasePath)
                                 + std::to_string(mip)
                                 + "_"
                                 + std::string(faceNames[face])
                                 + ".png";

                auto result = ImageLoader::load_image(path);
                if (!result)
                    return;

                mips[mip][face] = std::move(*result);
            }

        mPrefilteredEnvTexture = TextureAllocator::create_cubemap_with_mips(context, mips);
    }

    void SkyboxPass::CreateBrdfLutTexture(const Context &context)
    {
        if (auto data = ImageLoader::load_image(kBRDFLutPath))
            mBRDFLutTexture = TextureAllocator::create_from_image_data(context, *data);
    }
}
