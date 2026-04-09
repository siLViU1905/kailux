#include "SkyboxPass.h"

#include "descriptor/DescriptorSet.h"
#include "texture/ImageLoader.h"
#include "texture/TextureAllocator.h"

namespace kailux
{
    SkyboxPass::SkyboxPass() = default;

    SkyboxPass::SkyboxPass(SkyboxPass &&other) noexcept : m_DescriptorLayout(std::move(other.m_DescriptorLayout)),
                                                          m_Pipeline(std::move(other.m_Pipeline)),
                                                          m_DescriptorPool(std::move(other.m_DescriptorPool)),
                                                          m_Texture(std::move(other.m_Texture))
    {
    }

    SkyboxPass &SkyboxPass::operator=(SkyboxPass &&other) noexcept
    {
        if (this != &other)
        {
            m_DescriptorLayout = std::move(other.m_DescriptorLayout);
            m_Pipeline = std::move(other.m_Pipeline);
            m_DescriptorPool = std::move(other.m_DescriptorPool);
            m_Texture = std::move(other.m_Texture);
        }
        return *this;
    }

    SkyboxPass SkyboxPass::create(const Context &context, const Swapchain &swapchain, uint32_t sets,
                                  const std::array<std::string_view, 6> &paths)
    {
        SkyboxPass pass;
        pass.createDescriptorResources(context, sets);
        pass.createPipeline(context, swapchain);
        pass.createTexture(context, paths);
        return pass;
    }

    const Texture &SkyboxPass::getTexture() const
    {
        return m_Texture;
    }

    const DescriptorLayout &SkyboxPass::getDescriptorLayout() const
    {
        return m_DescriptorLayout;
    }

    const DescriptorPool &SkyboxPass::getDescriptorPool() const
    {
        return m_DescriptorPool;
    }

    void SkyboxPass::render(vk::CommandBuffer cmd, const DescriptorSet &descriptorSet, MeshView cubeView) const
    {
        m_Pipeline.bind(cmd);
        descriptorSet.bind(m_Pipeline, cmd);
        cmd.drawIndexed(
            cubeView.indexCount,
            1,
            cubeView.firstIndex,
            cubeView.vertexOffset,
            0
        );
    }

    PipelineInfo SkyboxPass::make_pipeline_info(vk::SampleCountFlagBits samples)
    {
        PipelineInfo info;

        info.vertexInputBinding = Vertex::get_binding_description();
        constexpr auto vertexAttribDesc = Vertex::get_attribute_description();
        info.vertexInputAttribute = {vertexAttribDesc.cbegin(), vertexAttribDesc.cend()};

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

        info.colorBlendAttachment.colorWriteMask =
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        info.colorBlendAttachment.blendEnable = vk::False;

        info.samples = samples;

        info.depthStencilInfo.depthTestEnable = vk::True;
        info.depthStencilInfo.depthWriteEnable = vk::False;
        info.depthStencilInfo.depthCompareOp = vk::CompareOp::eLessOrEqual;

        info.depthStencilInfo.depthBoundsTestEnable = vk::False;
        info.depthStencilInfo.stencilTestEnable = vk::False;

        return info;
    }

    void SkyboxPass::createDescriptorResources(const Context &context, uint32_t sets)
    {
        m_DescriptorLayout = DescriptorLayout::create(context, s_DescriptorLayoutBindings);
        m_DescriptorPool = DescriptorPool::create(context, sets, s_DescriptorPoolSizes);
    }

    void SkyboxPass::createPipeline(const Context &context, const Swapchain &swapchain)
    {
        m_Pipeline = Pipeline::create(
            context,
            swapchain,
            m_DescriptorLayout,
            {
                s_VertexShaderPath.data(),
                s_FragmentShaderPath.data()
            },
            make_pipeline_info(context.getMaxUsableSampleCount()));
    }

    void SkyboxPass::createTexture(const Context &context, const std::array<std::string_view, 6> &paths)
    {
        std::array<ImageLoader::ImageData, 6> faces;
        int i = 0;
        for (auto path: paths)
        {
            auto result = ImageLoader::load_image(path);
            if (!result)
                return;
            faces[i++] = *result;
        }

        m_Texture = TextureAllocator::create_cubemap(context, faces);
    }
}
