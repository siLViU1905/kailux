#include "FrameData.h"

#include "Pipeline.h"
#include "buffer/BufferAllocator.h"
#include "components/gpu/CameraData.h"
#include "components/gpu/MeshData.h"
#include "components/gpu/MeshMaterialData.h"
#include "components/gpu/MeshTransformData.h"
#include "components/gpu/SceneData.h"

namespace kailux
{
    FrameData::FrameData() : m_CommandPool({}),
                             m_ImGuiCommandPool({}),
                             m_CommandBuffer({}),
                             m_ImGuiCommandBuffer({}),
                             m_FenceInFlight({})

    {
    }

    FrameData::FrameData(FrameData &&other) noexcept : m_CommandPool(std::move(other.m_CommandPool)),
                                                       m_ImGuiCommandPool(std::move(other.m_ImGuiCommandPool)),
                                                       m_CommandBuffer(std::move(other.m_CommandBuffer)),
                                                       m_ImGuiCommandBuffer(std::move(other.m_ImGuiCommandBuffer)),
                                                       m_FenceInFlight(std::move(other.m_FenceInFlight)),
                                                       m_DescriptorSet(std::move(other.m_DescriptorSet)),
                                                       m_SkyboxDescriptorSet(std::move(other.m_SkyboxDescriptorSet)),
                                                       m_CameraBuffer(std::move(other.m_CameraBuffer)),
                                                       m_MeshDataBuffer(std::move(other.m_MeshDataBuffer)),
                                                       m_IndirectBuffer(std::move(other.m_IndirectBuffer)),
                                                       m_SceneBuffer(std::move(other.m_SceneBuffer))
    {
    }

    FrameData &FrameData::operator=(FrameData &&other) noexcept
    {
        if (this != &other)
        {
            m_CommandPool = std::move(other.m_CommandPool);
            m_ImGuiCommandPool = std::move(other.m_ImGuiCommandPool);
            m_CommandBuffer = std::move(other.m_CommandBuffer);
            m_ImGuiCommandBuffer = std::move(other.m_ImGuiCommandBuffer);
            m_FenceInFlight = std::move(other.m_FenceInFlight);
            m_DescriptorSet = std::move(other.m_DescriptorSet);
            m_SkyboxDescriptorSet = std::move(other.m_SkyboxDescriptorSet);
            m_CameraBuffer = std::move(other.m_CameraBuffer);
            m_MeshDataBuffer = std::move(other.m_MeshDataBuffer);
            m_IndirectBuffer = std::move(other.m_IndirectBuffer);
            m_SceneBuffer = std::move(other.m_SceneBuffer);
        }
        return *this;
    }

    FrameData FrameData::create(
        const Context &context,
        const DescriptorLayout &descriptorLayout,
        const DescriptorPool &descriptorPool,
        const SkyboxPass &skybox,
        const TextureRegistry &textureRegistry,
        uint32_t maxMeshCount
    )
    {
        FrameData frame;
        frame.createCommandPool(context);
        frame.createImGuiCommandPool(context);
        frame.createCommandBuffer(context);
        frame.createImGuiCommandBuffer(context);
        frame.createSyncObjects(context);
        frame.createCameraBuffer(context);
        frame.createMeshDataBuffer(context, maxMeshCount);
        frame.createIndirectBuffer(context, maxMeshCount);
        frame.createSceneBuffer(context);
        auto descSetInfo = frame.makeDescriptorSetInfo(skybox, textureRegistry, maxMeshCount);
        frame.createDescriptorSet(context, descriptorLayout, descriptorPool, descSetInfo);
        auto skyboxDescInfo = frame.makeSkyboxDescriptorSetInfo(skybox.getTexture());
        frame.createSkyboxDescriptorSet(context, skybox.getDescriptorLayout(), skybox.getDescriptorPool(), skyboxDescInfo);

        return frame;
    }

    void FrameData::reset(const Context &context) const
    {
        auto result = context.getDevice().waitForFences(*m_FenceInFlight, true, UINT64_MAX);
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("waitForFences failed");

        context.getDevice().resetFences(*m_FenceInFlight);

        m_CommandPool.reset();
    }

    vk::CommandBuffer FrameData::getCommandBuffer() const
    {
        return *m_CommandBuffer;
    }

    vk::CommandBuffer FrameData::getImGuiCommandBuffer() const
    {
        return *m_ImGuiCommandBuffer;
    }

    vk::Fence FrameData::getFenceInFlight() const
    {
        return *m_FenceInFlight;
    }

    const DescriptorSet &FrameData::getDescriptorSet() const
    {
        return m_DescriptorSet;
    }

    const DescriptorSet & FrameData::getSkyboxDescriptorSet() const
    {
        return m_SkyboxDescriptorSet;
    }

    Buffer &FrameData::getCameraBuffer()
    {
        return m_CameraBuffer;
    }

    Buffer &FrameData::getModelBuffer()
    {
        return m_MeshDataBuffer;
    }

    Buffer &FrameData::getIndirectBuffer()
    {
        return m_IndirectBuffer;
    }

    const Buffer &FrameData::getIndirectBuffer() const
    {
        return m_IndirectBuffer;
    }

    Buffer &FrameData::getSceneBuffer()
    {
        return m_SceneBuffer;
    }

    std::array<vk::BufferMemoryBarrier2, FrameData::s_BufferMemoryBarriersCount>
    FrameData::getBufferMemoryBarriers() const
    {
        return {
            vk::BufferMemoryBarrier2( // camera
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eVertexShader,
                vk::AccessFlagBits2::eUniformRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                m_CameraBuffer.getBuffer(),
                {},
                m_CameraBuffer.getSize()
            ),
            vk::BufferMemoryBarrier2( // model
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eVertexShader,
                vk::AccessFlagBits2::eShaderStorageRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                m_MeshDataBuffer.getBuffer(),
                {},
                m_MeshDataBuffer.getSize()
            ),
            vk::BufferMemoryBarrier2( // indirect
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eDrawIndirect,
                vk::AccessFlagBits2::eIndirectCommandRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                m_IndirectBuffer.getBuffer(),
                {},
                m_IndirectBuffer.getSize()
            ),
            vk::BufferMemoryBarrier2( // scene
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eFragmentShader,
                vk::AccessFlagBits2::eShaderStorageRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                m_SceneBuffer.getBuffer(),
                {},
                m_SceneBuffer.getSize()
            )
        };
    }

    void FrameData::createCommandPool(const Context &context)
    {
        vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        m_CommandPool = vk::raii::CommandPool(context.m_Device, poolInfo);
    }

    void FrameData::createImGuiCommandPool(const Context &context)
    {
        vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        m_ImGuiCommandPool = vk::raii::CommandPool(context.m_Device, poolInfo);
    }

    void FrameData::createCommandBuffer(const Context &context)
    {
        vk::CommandBufferAllocateInfo allocInfo(
            m_CommandPool,
            vk::CommandBufferLevel::ePrimary,
            1
        );

        m_CommandBuffer = std::move(vk::raii::CommandBuffers(context.m_Device, allocInfo).front());
    }

    void FrameData::createImGuiCommandBuffer(const Context &context)
    {
        vk::CommandBufferAllocateInfo allocInfo(
            m_CommandPool,
            vk::CommandBufferLevel::eSecondary,
            1
        );

        m_ImGuiCommandBuffer = std::move(vk::raii::CommandBuffers(context.m_Device, allocInfo).front());
    }

    void FrameData::createSyncObjects(const Context &context)
    {
        m_FenceInFlight = vk::raii::Fence(context.m_Device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }

    void FrameData::createDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                        const DescriptorPool &descriptorPool, std::span<const DescriptorSetInfo> infos)
    {
        m_DescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::createSkyboxDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                              const DescriptorPool &descriptorPool, std::span<const DescriptorSetInfo> infos)
    {
        m_SkyboxDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::createCameraBuffer(const Context &context)
    {
        m_CameraBuffer = BufferAllocator::alloc_uniform(context, sizeof(CameraData));
    }

    void FrameData::createMeshDataBuffer(const Context &context, uint32_t meshCount)
    {
        m_MeshDataBuffer = BufferAllocator::alloc_storage(context, meshCount * sizeof(MeshData));
    }

    void FrameData::createIndirectBuffer(const Context &context, uint32_t count)
    {
        m_IndirectBuffer = BufferAllocator::alloc_host(context, count * sizeof(vk::DrawIndexedIndirectCommand),
                                                       vk::BufferUsageFlagBits::eIndirectBuffer);
    }

    void FrameData::createSceneBuffer(const Context &context)
    {
        m_SceneBuffer = BufferAllocator::alloc_storage(context, sizeof(SceneData));
    }

    std::array<DescriptorSetInfo, FrameData::s_DescriptorSetInfoCount> FrameData::makeDescriptorSetInfo(const SkyboxPass &skybox, const TextureRegistry& textureRegistry, uint32_t meshCount) const
    {
        return {
            DescriptorSetBufferInfo(
                vk::DescriptorType::eUniformBuffer,
                m_CameraBuffer.getBuffer(),
                m_CameraBuffer.getSize(),
                1
            ),
            DescriptorSetBufferInfo(
                vk::DescriptorType::eStorageBuffer,
                m_MeshDataBuffer.getBuffer(),
                m_MeshDataBuffer.getSize(),
                1
            ),
            DescriptorSetBufferInfo(
                vk::DescriptorType::eStorageBuffer,
                m_SceneBuffer.getBuffer(),
                m_SceneBuffer.getSize(),
                1
            ),
            DescriptorSetImageInfo(
                skybox.getTexture().getSampler(),
                skybox.getTexture().getImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1
            ),
            DescriptorSetImageInfo(
                skybox.getIrradianceMapTexture().getSampler(),
                skybox.getIrradianceMapTexture().getImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1
            ),
            DescriptorSetImageInfo(
                skybox.getBRDFLutTexture().getSampler(),
                skybox.getBRDFLutTexture().getImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1
            ),
            DescriptorSetImageInfo(
                textureRegistry.view(textureRegistry.getDefaultSetHandle()).albedo->getSampler(),
                textureRegistry.view(textureRegistry.getDefaultSetHandle()).albedo->getImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                meshCount
            ),
            DescriptorSetImageInfo(
                textureRegistry.view(textureRegistry.getDefaultSetHandle()).normal->getSampler(),
                textureRegistry.view(textureRegistry.getDefaultSetHandle()).normal->getImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                meshCount
            ),
            DescriptorSetImageInfo(
                textureRegistry.view(textureRegistry.getDefaultSetHandle()).roughness->getSampler(),
                textureRegistry.view(textureRegistry.getDefaultSetHandle()).roughness->getImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                meshCount
            ),
            DescriptorSetImageInfo(
                textureRegistry.view(textureRegistry.getDefaultSetHandle()).metallic->getSampler(),
                textureRegistry.view(textureRegistry.getDefaultSetHandle()).metallic->getImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                meshCount
            ),
            DescriptorSetImageInfo(
                textureRegistry.view(textureRegistry.getDefaultSetHandle()).ao->getSampler(),
                textureRegistry.view(textureRegistry.getDefaultSetHandle()).ao->getImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                meshCount
            )
        };
    }

    std::array<DescriptorSetInfo, FrameData::s_SkyboxDescriptorSetInfoCount> FrameData::makeSkyboxDescriptorSetInfo(
        const Texture &skyboxTexture) const
    {
        return {
            DescriptorSetBufferInfo(
                vk::DescriptorType::eUniformBuffer,
                m_CameraBuffer.getBuffer(),
                m_CameraBuffer.getSize(),
                1
            ),
            DescriptorSetImageInfo(
                skyboxTexture.getSampler(),
                skyboxTexture.getImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1
            )
        };
    }
}
