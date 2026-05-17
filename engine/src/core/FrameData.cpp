#include "FrameData.h"

#include "Pipeline.h"
#include "buffer/BufferAllocator.h"
#include "components/gpu/CameraData.h"
#include "components/gpu/MeshData.h"
#include "components/gpu/MeshMaterialData.h"
#include "components/gpu/MeshTransformData.h"
#include "components/gpu/SceneData.h"
#include  "texture/TextureAllocator.h"

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
                                                       m_PickerDescriptorSet(std::move(other.m_PickerDescriptorSet)),
                                                       m_OutlineDescriptorSet(std::move(other.m_OutlineDescriptorSet)),
                                                       m_CameraBuffer(std::move(other.m_CameraBuffer)),
                                                       m_MeshDataBuffer(std::move(other.m_MeshDataBuffer)),
                                                       m_IndirectBuffer(std::move(other.m_IndirectBuffer)),
                                                       m_SceneBuffer(std::move(other.m_SceneBuffer)),
                                                       m_PickerBuffer(std::move(other.m_PickerBuffer)),
                                                       m_Extent(other.m_Extent),
                                                       m_SceneTexture(std::move(other.m_SceneTexture)),
                                                       m_OutIdTexture(std::move(other.m_OutIdTexture)),
                                                       m_ResolvedOutIdTexture(std::move(other.m_ResolvedOutIdTexture))
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
            m_PickerDescriptorSet = std::move(other.m_PickerDescriptorSet);
            m_OutlineDescriptorSet = std::move(other.m_OutlineDescriptorSet);
            m_CameraBuffer = std::move(other.m_CameraBuffer);
            m_MeshDataBuffer = std::move(other.m_MeshDataBuffer);
            m_IndirectBuffer = std::move(other.m_IndirectBuffer);
            m_SceneBuffer = std::move(other.m_SceneBuffer);
            m_PickerBuffer = std::move(other.m_PickerBuffer);
            m_Extent = other.m_Extent;
            m_SceneTexture = std::move(other.m_SceneTexture);
            m_OutIdTexture = std::move(other.m_OutIdTexture);
            m_ResolvedOutIdTexture = std::move(other.m_ResolvedOutIdTexture);
        }
        return *this;
    }

    FrameData FrameData::create(
        const Context &context,
        const Swapchain &swapchain,
        const DescriptorLayout &descriptorLayout,
        const DescriptorPool &descriptorPool,
        const SkyboxPass &skybox,
        const ComputePicker &picker,
        const OutlinePass &outlinePass,
        const TextureRegistry &textureRegistry, uint32_t maxMeshCount
    )
    {
        FrameData frame;
        frame.m_Extent = swapchain.getExtent();
        frame.createCommandPool(context);
        frame.createImGuiCommandPool(context);
        frame.createCommandBuffer(context);
        frame.createImGuiCommandBuffer(context);
        frame.createSyncObjects(context);
        frame.createCameraBuffer(context);
        frame.createMeshDataBuffer(context, maxMeshCount);
        frame.createIndirectBuffer(context, maxMeshCount);
        frame.createSceneBuffer(context);
        frame.createPickerBuffer(context);
        frame.createSceneTexture(context, swapchain.getFormat());
        frame.createOutIdTexture(context);
        auto descSetInfo = frame.makeDescriptorSetInfo(skybox, textureRegistry, maxMeshCount);
        frame.createDescriptorSet(context, descriptorLayout, descriptorPool, descSetInfo);
        auto skyboxDescInfo = frame.makeSkyboxDescriptorSetInfo(skybox.getTexture());
        frame.createSkyboxDescriptorSet(context, skybox.getDescriptorLayout(), skybox.getDescriptorPool(),
                                        skyboxDescInfo);
        auto pickerDescInfo = frame.makePickerDescriptorSetInfo();
        frame.createPickerDescriptorSet(context, picker.getDescriptorLayout(), picker.getDescriptorPool(),
                                        pickerDescInfo);
        auto outlineDescInfo = frame.makeOutlineDescriptorSetInfo();
        frame.createOutlineDescriptorSet(context, outlinePass.getDescriptorLayout(), outlinePass.getDescriptorPool(),
                                         outlineDescInfo);
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

    void FrameData::recreateTextures(const Context &context, const Swapchain &swapchain)
    {
        m_Extent = swapchain.getExtent();

        createSceneTexture(context, swapchain.getFormat());

        createOutIdTexture(context);
        std::array pickerInfo{
            DescriptorSetUpdateInfo(s_PickerResolvedViewDescriptorSetBinding,
                                    0,
                                    DescriptorSetImageInfo(
                                        nullptr,
                                        m_ResolvedOutIdTexture.getImageView(),
                                        vk::ImageLayout::eGeneral,
                                        1,
                                        vk::DescriptorType::eStorageImage
                                    ))
        };
        m_PickerDescriptorSet.updateInfo(context, pickerInfo);

        std::array outlineInfo{
            DescriptorSetUpdateInfo(s_OutlineIdResolvedViewDescriptorSetBinding,
                                    0,
                                    DescriptorSetImageInfo(
                                        m_ResolvedOutIdTexture.getSampler(),
                                        m_ResolvedOutIdTexture.getImageView(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        1,
                                        vk::DescriptorType::eCombinedImageSampler
                                    ))
        };
        m_OutlineDescriptorSet.updateInfo(context, outlineInfo);
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

    const DescriptorSet &FrameData::getSkyboxDescriptorSet() const
    {
        return m_SkyboxDescriptorSet;
    }

    const DescriptorSet &FrameData::getPickerDescriptorSet() const
    {
        return m_PickerDescriptorSet;
    }

    const DescriptorSet &FrameData::getOutlineDescriptorSet() const
    {
        return m_OutlineDescriptorSet;
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

    const Buffer &FrameData::getPickerBuffer() const
    {
        return m_PickerBuffer;
    }

    vk::Extent2D FrameData::getExtent() const
    {
        return m_Extent;
    }

    const Texture &FrameData::getSceneTexture() const
    {
        return m_SceneTexture;
    }

    const Texture &FrameData::getOutIdTexture() const
    {
        return m_OutIdTexture;
    }

    const Texture &FrameData::getResolvedOutIdTexture() const
    {
        return m_ResolvedOutIdTexture;
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

    vk::BufferMemoryBarrier2 FrameData::getPickerBufferMemoryBarrier() const
    {
        return {
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eShaderWrite,
            vk::PipelineStageFlagBits2::eHost,
            vk::AccessFlagBits2::eHostRead,
            vk::QueueFamilyIgnored,
            vk::QueueFamilyIgnored,
            m_PickerBuffer.getBuffer(),
            {},
            sizeof(uint32_t)
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
                                              const DescriptorPool &descriptorPool,
                                              std::span<const DescriptorSetInfo> infos)
    {
        m_SkyboxDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::createPickerDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                              const DescriptorPool &descriptorPool,
                                              std::span<const DescriptorSetInfo> infos)
    {
        m_PickerDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::createOutlineDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                               const DescriptorPool &descriptorPool,
                                               std::span<const DescriptorSetInfo> infos)
    {
        m_OutlineDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
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

    void FrameData::createPickerBuffer(const Context &context)
    {
        m_PickerBuffer = BufferAllocator::alloc_storage(context, sizeof(uint32_t));
    }

    void FrameData::createSceneTexture(const Context &context, vk::Format format)
    {
        m_SceneTexture = TextureAllocator::create_empty(
            context,
            m_Extent.width,
            m_Extent.height,
            format,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            vk::ImageAspectFlagBits::eColor,
            vk::SampleCountFlagBits::e1
        );
    }

    void FrameData::createOutIdTexture(const Context &context)
    {
        m_OutIdTexture = TextureAllocator::create_empty(
            context,
            m_Extent.width,
            m_Extent.height,
            vk::Format::eR32Uint,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::ImageAspectFlagBits::eColor,
            context.getMaxUsableSampleCount()
        );
        m_ResolvedOutIdTexture = TextureAllocator::create_empty(
            context,
            m_Extent.width,
            m_Extent.height,
            vk::Format::eR32Uint,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage |
            vk::ImageUsageFlagBits::eSampled,
            vk::ImageAspectFlagBits::eColor,
            vk::SampleCountFlagBits::e1
        );
    }

    std::array<DescriptorSetInfo, FrameData::s_DescriptorSetInfoCount> FrameData::makeDescriptorSetInfo(
        const SkyboxPass &skybox, const TextureRegistry &textureRegistry, uint32_t meshCount) const
    {
        return {
            DescriptorSetBufferInfo(
                m_CameraBuffer.getBuffer(),
                m_CameraBuffer.getSize(),
                1,
                vk::DescriptorType::eUniformBuffer
            ),
            DescriptorSetBufferInfo(
                m_MeshDataBuffer.getBuffer(),
                m_MeshDataBuffer.getSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                m_SceneBuffer.getBuffer(),
                m_SceneBuffer.getSize(),
                1,
                vk::DescriptorType::eStorageBuffer
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
                skybox.getPrefilteredEnvTexture().getSampler(),
                skybox.getPrefilteredEnvTexture().getImageView(),
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
                m_CameraBuffer.getBuffer(),
                m_CameraBuffer.getSize(),
                1,
                vk::DescriptorType::eUniformBuffer
            ),
            DescriptorSetImageInfo(
                skyboxTexture.getSampler(),
                skyboxTexture.getImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1
            )
        };
    }

    std::array<DescriptorSetInfo, FrameData::s_PickerDescriptorSetInfoCount>
    FrameData::makePickerDescriptorSetInfo() const
    {
        return {
            DescriptorSetImageInfo(
                nullptr,
                m_ResolvedOutIdTexture.getImageView(),
                vk::ImageLayout::eGeneral,
                1,
                vk::DescriptorType::eStorageImage
            ),
            DescriptorSetBufferInfo(
                m_PickerBuffer.getBuffer(),
                m_PickerBuffer.getSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            )
        };
    }

    std::array<DescriptorSetInfo, FrameData::s_OutlineDescriptorSetInfoCount> FrameData::
    makeOutlineDescriptorSetInfo() const
    {
        return {
            DescriptorSetImageInfo(
                m_ResolvedOutIdTexture.getSampler(),
                m_ResolvedOutIdTexture.getImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1,
                vk::DescriptorType::eCombinedImageSampler
            )
        };
    }
}
