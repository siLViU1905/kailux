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
    FrameData::FrameData() : mCommandPool({}),
                             mImGuiCommandPool({}),
                             mCommandBuffer({}),
                             mImGuiCommandBuffer({}),
                             mFenceInFlight({})

    {
    }

    FrameData::FrameData(FrameData &&other) noexcept : mCommandPool(std::move(other.mCommandPool)),
                                                       mImGuiCommandPool(std::move(other.mImGuiCommandPool)),
                                                       mCommandBuffer(std::move(other.mCommandBuffer)),
                                                       mImGuiCommandBuffer(std::move(other.mImGuiCommandBuffer)),
                                                       mFenceInFlight(std::move(other.mFenceInFlight)),
                                                       mDescriptorSet(std::move(other.mDescriptorSet)),
                                                       mSkyboxDescriptorSet(std::move(other.mSkyboxDescriptorSet)),
                                                       mGizmoDescriptorSet(std::move(other.mGizmoDescriptorSet)),
                                                       mPickerDescriptorSet(std::move(other.mPickerDescriptorSet)),
                                                       mOutlineDescriptorSet(std::move(other.mOutlineDescriptorSet)),
                                                       mCullerDescriptorSet(std::move(other.mCullerDescriptorSet)),
                                                       mCameraBuffer(std::move(other.mCameraBuffer)),
                                                       mMeshDataBuffer(std::move(other.mMeshDataBuffer)),
                                                       mIndirectBuffer(std::move(other.mIndirectBuffer)),
                                                       mSceneBuffer(std::move(other.mSceneBuffer)),
                                                       mPickerBuffer(std::move(other.mPickerBuffer)),
                                                       mCullerInputCommandsBuffer(
                                                           std::move(other.mCullerInputCommandsBuffer)),
                                                       mCullerCountBuffer(std::move(other.mCullerCountBuffer)),
                                                       mExtent(other.mExtent),
                                                       mSceneTexture(std::move(other.mSceneTexture)),
                                                       mOutIdTexture(std::move(other.mOutIdTexture)),
                                                       mResolvedOutIdTexture(std::move(other.mResolvedOutIdTexture))
    {
    }

    FrameData &FrameData::operator=(FrameData &&other) noexcept
    {
        if (this != &other)
        {
            mCommandPool = std::move(other.mCommandPool);
            mImGuiCommandPool = std::move(other.mImGuiCommandPool);
            mCommandBuffer = std::move(other.mCommandBuffer);
            mImGuiCommandBuffer = std::move(other.mImGuiCommandBuffer);
            mFenceInFlight = std::move(other.mFenceInFlight);
            mDescriptorSet = std::move(other.mDescriptorSet);
            mSkyboxDescriptorSet = std::move(other.mSkyboxDescriptorSet);
            mGizmoDescriptorSet = std::move(other.mGizmoDescriptorSet);
            mPickerDescriptorSet = std::move(other.mPickerDescriptorSet);
            mOutlineDescriptorSet = std::move(other.mOutlineDescriptorSet);
            mCullerDescriptorSet = std::move(other.mCullerDescriptorSet);
            mCameraBuffer = std::move(other.mCameraBuffer);
            mMeshDataBuffer = std::move(other.mMeshDataBuffer);
            mIndirectBuffer = std::move(other.mIndirectBuffer);
            mSceneBuffer = std::move(other.mSceneBuffer);
            mPickerBuffer = std::move(other.mPickerBuffer);
            mCullerInputCommandsBuffer = std::move(other.mCullerInputCommandsBuffer);
            mCullerCountBuffer = std::move(other.mCullerCountBuffer);
            mExtent = other.mExtent;
            mSceneTexture = std::move(other.mSceneTexture);
            mOutIdTexture = std::move(other.mOutIdTexture);
            mResolvedOutIdTexture = std::move(other.mResolvedOutIdTexture);
        }
        return *this;
    }

    FrameData FrameData::create(
        const Context &context,
        const Swapchain &swapchain,
        const MainPass &mainPass,
        const SkyboxPass &skybox,
        const GizmoPass & gizmoPass,
        const ComputePicker &picker,
        const OutlinePass &outlinePass, const ComputeCuller &culler, const TextureRegistry &textureRegistry, uint32_t maxMeshCount
    )
    {
        FrameData frame;
        frame.mExtent = swapchain.getExtent();
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
        frame.createCullerBuffers(context, maxMeshCount);
        frame.createSceneTexture(context, swapchain.getFormat());
        frame.createOutIdTexture(context);
        auto descSetInfo = frame.makeDescriptorSetInfo(skybox, textureRegistry, maxMeshCount);
        frame.createDescriptorSet(context, mainPass.getDescriptorLayout(), mainPass.getDescriptorPool(), descSetInfo);
        auto skyboxDescInfo = frame.makeSkyboxDescriptorSetInfo(skybox.getTexture());
        frame.createSkyboxDescriptorSet(context, skybox.getDescriptorLayout(), skybox.getDescriptorPool(),
                                        skyboxDescInfo);
        auto gizmoDescInfo = frame.makeGizmoDescriptorSetInfo();
        frame.createGizmoDescriptorSet(context, gizmoPass.getDescriptorLayout(), gizmoPass.getDescriptorPool(), gizmoDescInfo);
        auto pickerDescInfo = frame.makePickerDescriptorSetInfo();
        frame.createPickerDescriptorSet(context, picker.getDescriptorLayout(), picker.getDescriptorPool(),
                                        pickerDescInfo);
        auto outlineDescInfo = frame.makeOutlineDescriptorSetInfo();
        frame.createOutlineDescriptorSet(context, outlinePass.getDescriptorLayout(), outlinePass.getDescriptorPool(),
                                         outlineDescInfo);
        auto cullerDescInfo = frame.makeCullerDescriptorSetInfo();
        frame.createCullerDescriptorSet(context, culler.getDescriptorLayout(), culler.getDescriptorPool(),
                                        cullerDescInfo);
        return frame;
    }

    void FrameData::reset(const Context &context) const
    {
        auto result = context.getDevice().waitForFences(*mFenceInFlight, true, UINT64_MAX);
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("waitForFences failed");

        context.getDevice().resetFences(*mFenceInFlight);

        mCommandPool.reset();
    }

    void FrameData::recreateTextures(const Context &context, const Swapchain &swapchain)
    {
        mExtent = swapchain.getExtent();

        createSceneTexture(context, swapchain.getFormat());

        createOutIdTexture(context);
        std::array pickerInfo{
            DescriptorSetUpdateInfo(kPickerResolvedViewDescriptorSetBinding,
                                    0,
                                    DescriptorSetImageInfo(
                                        nullptr,
                                        mResolvedOutIdTexture.getImageView(),
                                        vk::ImageLayout::eGeneral,
                                        1,
                                        vk::DescriptorType::eStorageImage
                                    ))
        };
        mPickerDescriptorSet.updateInfo(context, pickerInfo);

        std::array outlineInfo{
            DescriptorSetUpdateInfo(kOutlineIdResolvedViewDescriptorSetBinding,
                                    0,
                                    DescriptorSetImageInfo(
                                        mResolvedOutIdTexture.getSampler(),
                                        mResolvedOutIdTexture.getImageView(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        1,
                                        vk::DescriptorType::eCombinedImageSampler
                                    ))
        };
        mOutlineDescriptorSet.updateInfo(context, outlineInfo);
    }

    vk::CommandBuffer FrameData::getCommandBuffer() const
    {
        return *mCommandBuffer;
    }

    vk::CommandBuffer FrameData::getImGuiCommandBuffer() const
    {
        return *mImGuiCommandBuffer;
    }

    vk::Fence FrameData::getFenceInFlight() const
    {
        return *mFenceInFlight;
    }

    const DescriptorSet &FrameData::getDescriptorSet() const
    {
        return mDescriptorSet;
    }

    const DescriptorSet &FrameData::getSkyboxDescriptorSet() const
    {
        return mSkyboxDescriptorSet;
    }

    const DescriptorSet & FrameData::getGizmoDescriptorSet() const
    {
        return mGizmoDescriptorSet;
    }

    const DescriptorSet &FrameData::getPickerDescriptorSet() const
    {
        return mPickerDescriptorSet;
    }

    const DescriptorSet &FrameData::getOutlineDescriptorSet() const
    {
        return mOutlineDescriptorSet;
    }

    const DescriptorSet &FrameData::getCullerDescriptorSet() const
    {
        return mCullerDescriptorSet;
    }

    Buffer &FrameData::getCameraBuffer()
    {
        return mCameraBuffer;
    }

    Buffer &FrameData::getModelBuffer()
    {
        return mMeshDataBuffer;
    }

    Buffer &FrameData::getIndirectBuffer()
    {
        return mIndirectBuffer;
    }

    const Buffer &FrameData::getIndirectBuffer() const
    {
        return mIndirectBuffer;
    }

    Buffer &FrameData::getSceneBuffer()
    {
        return mSceneBuffer;
    }

    const Buffer &FrameData::getPickerBuffer() const
    {
        return mPickerBuffer;
    }

    const Buffer &FrameData::getCullerInputCommandsBuffer() const
    {
        return mCullerInputCommandsBuffer;
    }

    const Buffer &FrameData::getCullerCountBuffer() const
    {
        return mCullerCountBuffer;
    }

    vk::Extent2D FrameData::getExtent() const
    {
        return mExtent;
    }

    const Texture &FrameData::getSceneTexture() const
    {
        return mSceneTexture;
    }

    const Texture &FrameData::getOutIdTexture() const
    {
        return mOutIdTexture;
    }

    const Texture &FrameData::getResolvedOutIdTexture() const
    {
        return mResolvedOutIdTexture;
    }

    std::array<vk::BufferMemoryBarrier2, FrameData::kBufferMemoryBarriersCount>
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
                mCameraBuffer.getBuffer(),
                {},
                mCameraBuffer.getSize()
            ),
            vk::BufferMemoryBarrier2( // model
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eVertexShader,
                vk::AccessFlagBits2::eShaderStorageRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mMeshDataBuffer.getBuffer(),
                {},
                mMeshDataBuffer.getSize()
            ),
            vk::BufferMemoryBarrier2( // culler input
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderStorageRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mCullerInputCommandsBuffer.getBuffer(),
                {},
                mCullerInputCommandsBuffer.getSize()
            ),
            vk::BufferMemoryBarrier2( // scene
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eFragmentShader,
                vk::AccessFlagBits2::eShaderStorageRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mSceneBuffer.getBuffer(),
                {},
                mSceneBuffer.getSize()
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
            mPickerBuffer.getBuffer(),
            {},
            sizeof(uint32_t)
        };
    }

    std::array<vk::BufferMemoryBarrier2, FrameData::kCullerBufferMemoryBarriersCount> FrameData::
    getCullerBufferMemoryBarriers() const
    {
        return {
            vk::BufferMemoryBarrier2( // indirect buffer
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eDrawIndirect,
                vk::AccessFlagBits2::eIndirectCommandRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mIndirectBuffer.getBuffer(),
                {},
                mIndirectBuffer.getSize()
            ),
            vk::BufferMemoryBarrier2( // culler count
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eDrawIndirect,
                vk::AccessFlagBits2::eIndirectCommandRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mCullerCountBuffer.getBuffer(),
                {},
                mCullerCountBuffer.getSize()
            )
        };
    }

    vk::BufferMemoryBarrier2 FrameData::getCullerCountBufferFillMemoryBarrier() const
    {
        return {
            vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eTransferWrite,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite,
            vk::QueueFamilyIgnored,
            vk::QueueFamilyIgnored,
            mCullerCountBuffer.getBuffer(),
            {},
            mCullerCountBuffer.getSize()
        };
    }

    void FrameData::createCommandPool(const Context &context)
    {
        vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        mCommandPool = vk::raii::CommandPool(context.mDevice, poolInfo);
    }

    void FrameData::createImGuiCommandPool(const Context &context)
    {
        vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        mImGuiCommandPool = vk::raii::CommandPool(context.mDevice, poolInfo);
    }

    void FrameData::createCommandBuffer(const Context &context)
    {
        vk::CommandBufferAllocateInfo allocInfo(
            mCommandPool,
            vk::CommandBufferLevel::ePrimary,
            1
        );

        mCommandBuffer = std::move(vk::raii::CommandBuffers(context.mDevice, allocInfo).front());
    }

    void FrameData::createImGuiCommandBuffer(const Context &context)
    {
        vk::CommandBufferAllocateInfo allocInfo(
            mCommandPool,
            vk::CommandBufferLevel::eSecondary,
            1
        );

        mImGuiCommandBuffer = std::move(vk::raii::CommandBuffers(context.mDevice, allocInfo).front());
    }

    void FrameData::createSyncObjects(const Context &context)
    {
        mFenceInFlight = vk::raii::Fence(context.mDevice, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }

    void FrameData::createDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                        const DescriptorPool &descriptorPool, std::span<const DescriptorSetInfo> infos)
    {
        mDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::createSkyboxDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                              const DescriptorPool &descriptorPool,
                                              std::span<const DescriptorSetInfo> infos)
    {
        mSkyboxDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::createGizmoDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
        const DescriptorPool &descriptorPool, std::span<const DescriptorSetInfo> infos)
    {
        mGizmoDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::createPickerDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                              const DescriptorPool &descriptorPool,
                                              std::span<const DescriptorSetInfo> infos)
    {
        mPickerDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::createOutlineDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                               const DescriptorPool &descriptorPool,
                                               std::span<const DescriptorSetInfo> infos)
    {
        mOutlineDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::createCullerDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                              const DescriptorPool &descriptorPool,
                                              std::span<const DescriptorSetInfo> infos)
    {
        mCullerDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::createCameraBuffer(const Context &context)
    {
        mCameraBuffer = BufferAllocator::alloc_uniform(context, sizeof(CameraData));
    }

    void FrameData::createMeshDataBuffer(const Context &context, uint32_t meshCount)
    {
        mMeshDataBuffer = BufferAllocator::alloc_storage(context, meshCount * sizeof(MeshData));
    }

    void FrameData::createIndirectBuffer(const Context &context, uint32_t count)
    {
        mIndirectBuffer = BufferAllocator::alloc_host(context, count * sizeof(vk::DrawIndexedIndirectCommand),
                                                       vk::BufferUsageFlagBits::eIndirectBuffer |
                                                       vk::BufferUsageFlagBits::eStorageBuffer);
    }

    void FrameData::createSceneBuffer(const Context &context)
    {
        mSceneBuffer = BufferAllocator::alloc_storage(context, sizeof(SceneData));
    }

    void FrameData::createPickerBuffer(const Context &context)
    {
        mPickerBuffer = BufferAllocator::alloc_storage(context, sizeof(uint32_t));
    }

    void FrameData::createCullerBuffers(const Context &context, uint32_t count)
    {
        mCullerInputCommandsBuffer = BufferAllocator::alloc_host(context, count * sizeof(vk::DrawIndexedIndirectCommand),
                                                                  vk::BufferUsageFlagBits::eStorageBuffer);
        mCullerCountBuffer = BufferAllocator::alloc_local(context, sizeof(uint32_t),
                                                           vk::BufferUsageFlagBits::eStorageBuffer |
                                                           vk::BufferUsageFlagBits::eIndirectBuffer);
    }

    void FrameData::createSceneTexture(const Context &context, vk::Format format)
    {
        mSceneTexture = TextureAllocator::create_empty(
            context,
            mExtent.width,
            mExtent.height,
            format,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            vk::ImageAspectFlagBits::eColor,
            vk::SampleCountFlagBits::e1
        );
    }

    void FrameData::createOutIdTexture(const Context &context)
    {
        mOutIdTexture = TextureAllocator::create_empty(
            context,
            mExtent.width,
            mExtent.height,
            vk::Format::eR32Uint,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::ImageAspectFlagBits::eColor,
            context.getMaxUsableSampleCount()
        );
        mResolvedOutIdTexture = TextureAllocator::create_empty(
            context,
            mExtent.width,
            mExtent.height,
            vk::Format::eR32Uint,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage |
            vk::ImageUsageFlagBits::eSampled,
            vk::ImageAspectFlagBits::eColor,
            vk::SampleCountFlagBits::e1
        );
    }

    std::array<DescriptorSetInfo, FrameData::kDescriptorSetInfoCount> FrameData::makeDescriptorSetInfo(
        const SkyboxPass &skybox, const TextureRegistry &textureRegistry, uint32_t meshCount) const
    {
        return {
            DescriptorSetBufferInfo(
                mCameraBuffer.getBuffer(),
                mCameraBuffer.getSize(),
                1,
                vk::DescriptorType::eUniformBuffer
            ),
            DescriptorSetBufferInfo(
                mMeshDataBuffer.getBuffer(),
                mMeshDataBuffer.getSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                mSceneBuffer.getBuffer(),
                mSceneBuffer.getSize(),
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

    std::array<DescriptorSetInfo, FrameData::kSkyboxDescriptorSetInfoCount> FrameData::makeSkyboxDescriptorSetInfo(
        const Texture &skyboxTexture) const
    {
        return {
            DescriptorSetBufferInfo(
                mCameraBuffer.getBuffer(),
                mCameraBuffer.getSize(),
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

    std::array<DescriptorSetInfo, FrameData::kGizmoDescriptorSetInfoCount> FrameData::makeGizmoDescriptorSetInfo() const
    {
        return {
            DescriptorSetBufferInfo(
                mCameraBuffer.getBuffer(),
                mCameraBuffer.getSize(),
                1,
                vk::DescriptorType::eUniformBuffer
                )
        };
    }

    std::array<DescriptorSetInfo, FrameData::kPickerDescriptorSetInfoCount>
    FrameData::makePickerDescriptorSetInfo() const
    {
        return {
            DescriptorSetImageInfo(
                nullptr,
                mResolvedOutIdTexture.getImageView(),
                vk::ImageLayout::eGeneral,
                1,
                vk::DescriptorType::eStorageImage
            ),
            DescriptorSetBufferInfo(
                mPickerBuffer.getBuffer(),
                mPickerBuffer.getSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            )
        };
    }

    std::array<DescriptorSetInfo, FrameData::kOutlineDescriptorSetInfoCount> FrameData::
    makeOutlineDescriptorSetInfo() const
    {
        return {
            DescriptorSetImageInfo(
                mResolvedOutIdTexture.getSampler(),
                mResolvedOutIdTexture.getImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1,
                vk::DescriptorType::eCombinedImageSampler
            )
        };
    }

    std::array<DescriptorSetInfo, FrameData::kCullerDescriptorSetInfoCount> FrameData::
    makeCullerDescriptorSetInfo() const
    {
        return {
            DescriptorSetBufferInfo(
                mMeshDataBuffer.getBuffer(),
                mMeshDataBuffer.getSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                mCullerInputCommandsBuffer.getBuffer(),
                mCullerInputCommandsBuffer.getSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                mIndirectBuffer.getBuffer(),
                mIndirectBuffer.getSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                mCullerCountBuffer.getBuffer(),
                mCullerCountBuffer.getSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            )
        };
    }
}
