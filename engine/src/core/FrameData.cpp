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
                                                       mMeshDescriptorSet(std::move(other.mMeshDescriptorSet)),
                                                       mSkyboxDescriptorSet(std::move(other.mSkyboxDescriptorSet)),
                                                       mGizmoDescriptorSet(std::move(other.mGizmoDescriptorSet)),
                                                       mPickerDescriptorSet(std::move(other.mPickerDescriptorSet)),
                                                       mOutlineDescriptorSet(std::move(other.mOutlineDescriptorSet)),
                                                       mCullerDescriptorSet(std::move(other.mCullerDescriptorSet)),
                                                       mCameraBuffer(std::move(other.mCameraBuffer)),
                                                       mMeshDataBuffer(std::move(other.mMeshDataBuffer)),
                                                       mMaterialsBuffer(std::move(other.mMaterialsBuffer)),
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
            mMeshDescriptorSet = std::move(other.mMeshDescriptorSet);
            mSkyboxDescriptorSet = std::move(other.mSkyboxDescriptorSet);
            mGizmoDescriptorSet = std::move(other.mGizmoDescriptorSet);
            mPickerDescriptorSet = std::move(other.mPickerDescriptorSet);
            mOutlineDescriptorSet = std::move(other.mOutlineDescriptorSet);
            mCullerDescriptorSet = std::move(other.mCullerDescriptorSet);
            mCameraBuffer = std::move(other.mCameraBuffer);
            mMeshDataBuffer = std::move(other.mMeshDataBuffer);
            mMaterialsBuffer = std::move(other.mMaterialsBuffer);
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
        const OutlinePass &outlinePass,
        const ComputeCuller &culler,
        const TextureRegistry &textureRegistry
    )
    {
        FrameData frame;
        frame.mExtent = swapchain.GetExtent();
        frame.CreateCommandPool(context);
        frame.CreateImGuiCommandPool(context);
        frame.CreateCommandBuffer(context);
        frame.CreateImGuiCommandBuffer(context);
        frame.CreateSyncObjects(context);
        frame.CreateCameraBuffer(context);
        frame.CreateMeshDataBuffer(context);
        frame.CreateMaterialsBuffer(context);
        frame.CreateIndirectBuffer(context);
        frame.CreateSceneBuffer(context);
        frame.CreatePickerBuffer(context);
        frame.CreateCullerBuffers(context);
        frame.CreateSceneTexture(context, swapchain.GetFormat());
        frame.CreateOutIdTexture(context);
        auto descSetInfo = frame.MakeMeshDescriptorSetInfo(skybox, textureRegistry);
        frame.CreateMeshDescriptorSet(context, mainPass.GetDescriptorLayout(), mainPass.GetDescriptorPool(), descSetInfo);
        auto skyboxDescInfo = frame.MakeSkyboxDescriptorSetInfo(skybox.GetTexture());
        frame.CreateSkyboxDescriptorSet(context, skybox.GetDescriptorLayout(), skybox.GetDescriptorPool(),
                                        skyboxDescInfo);
        auto gizmoDescInfo = frame.MakeGizmoDescriptorSetInfo();
        frame.CreateGizmoDescriptorSet(context, gizmoPass.GetDescriptorLayout(), gizmoPass.GetDescriptorPool(), gizmoDescInfo);
        auto pickerDescInfo = frame.MakePickerDescriptorSetInfo();
        frame.CreatePickerDescriptorSet(context, picker.GetDescriptorLayout(), picker.GetDescriptorPool(),
                                        pickerDescInfo);
        auto outlineDescInfo = frame.MakeOutlineDescriptorSetInfo();
        frame.CreateOutlineDescriptorSet(context, outlinePass.GetDescriptorLayout(), outlinePass.GetDescriptorPool(),
                                         outlineDescInfo);
        auto cullerDescInfo = frame.MakeCullerDescriptorSetInfo();
        frame.CreateCullerDescriptorSet(context, culler.GetDescriptorLayout(), culler.GetDescriptorPool(),
                                        cullerDescInfo);
        return frame;
    }

    void FrameData::Reset(const Context &context) const
    {
        auto result = context.GetDevice().waitForFences(*mFenceInFlight, true, UINT64_MAX);
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("waitForFences failed");

        context.GetDevice().resetFences(*mFenceInFlight);

        mCommandPool.reset();
    }

    void FrameData::RecreateTextures(const Context &context, const Swapchain &swapchain)
    {
        mExtent = swapchain.GetExtent();

        CreateSceneTexture(context, swapchain.GetFormat());

        CreateOutIdTexture(context);
        std::array pickerInfo{
            DescriptorSetUpdateInfo(kPickerResolvedViewDescriptorSetBinding,
                                    0,
                                    DescriptorSetImageInfo(
                                        nullptr,
                                        mResolvedOutIdTexture.GetImageView(),
                                        vk::ImageLayout::eGeneral,
                                        1,
                                        vk::DescriptorType::eStorageImage
                                    ))
        };
        mPickerDescriptorSet.UpdateInfo(context, pickerInfo);

        std::array outlineInfo{
            DescriptorSetUpdateInfo(kOutlineIdResolvedViewDescriptorSetBinding,
                                    0,
                                    DescriptorSetImageInfo(
                                        mResolvedOutIdTexture.GetSampler(),
                                        mResolvedOutIdTexture.GetImageView(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        1,
                                        vk::DescriptorType::eCombinedImageSampler
                                    ))
        };
        mOutlineDescriptorSet.UpdateInfo(context, outlineInfo);
    }

    vk::CommandBuffer FrameData::GetCommandBuffer() const
    {
        return *mCommandBuffer;
    }

    vk::CommandBuffer FrameData::GetImGuiCommandBuffer() const
    {
        return *mImGuiCommandBuffer;
    }

    vk::Fence FrameData::GetFenceInFlight() const
    {
        return *mFenceInFlight;
    }

    const DescriptorSet &FrameData::GetMeshDescriptorSet() const
    {
        return mMeshDescriptorSet;
    }

    const DescriptorSet &FrameData::GetSkyboxDescriptorSet() const
    {
        return mSkyboxDescriptorSet;
    }

    const DescriptorSet & FrameData::GetGizmoDescriptorSet() const
    {
        return mGizmoDescriptorSet;
    }

    const DescriptorSet &FrameData::GetPickerDescriptorSet() const
    {
        return mPickerDescriptorSet;
    }

    const DescriptorSet &FrameData::GetOutlineDescriptorSet() const
    {
        return mOutlineDescriptorSet;
    }

    const DescriptorSet &FrameData::GetCullerDescriptorSet() const
    {
        return mCullerDescriptorSet;
    }

    Buffer &FrameData::GetCameraBuffer()
    {
        return mCameraBuffer;
    }

    Buffer &FrameData::GetModelBuffer()
    {
        return mMeshDataBuffer;
    }

    Buffer & FrameData::GetMaterialBuffer()
    {
        return mMaterialsBuffer;
    }

    Buffer &FrameData::GetIndirectBuffer()
    {
        return mIndirectBuffer;
    }

    const Buffer &FrameData::GetIndirectBuffer() const
    {
        return mIndirectBuffer;
    }

    Buffer &FrameData::GetSceneBuffer()
    {
        return mSceneBuffer;
    }

    const Buffer &FrameData::GetPickerBuffer() const
    {
        return mPickerBuffer;
    }

    const Buffer &FrameData::GetCullerInputCommandsBuffer() const
    {
        return mCullerInputCommandsBuffer;
    }

    const Buffer &FrameData::GetCullerCountBuffer() const
    {
        return mCullerCountBuffer;
    }

    vk::Extent2D FrameData::GetExtent() const
    {
        return mExtent;
    }

    const Texture &FrameData::GetSceneTexture() const
    {
        return mSceneTexture;
    }

    const Texture &FrameData::GetOutIdTexture() const
    {
        return mOutIdTexture;
    }

    const Texture &FrameData::GetResolvedOutIdTexture() const
    {
        return mResolvedOutIdTexture;
    }

    std::array<vk::BufferMemoryBarrier2, FrameData::kBufferMemoryBarriersCount>
    FrameData::GetBufferMemoryBarriers() const
    {
        return {
            vk::BufferMemoryBarrier2( // camera
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eVertexShader,
                vk::AccessFlagBits2::eUniformRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mCameraBuffer.GetBuffer(),
                {},
                mCameraBuffer.GetSize()
            ),
            vk::BufferMemoryBarrier2( // model
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eVertexShader,
                vk::AccessFlagBits2::eShaderStorageRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mMeshDataBuffer.GetBuffer(),
                {},
                mMeshDataBuffer.GetSize()
            ),
            vk::BufferMemoryBarrier2( // materials
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eFragmentShader,
                vk::AccessFlagBits2::eShaderStorageRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mMaterialsBuffer.GetBuffer(),
                {},
                mMaterialsBuffer.GetSize()
            ),
            vk::BufferMemoryBarrier2( // culler input
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderStorageRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mCullerInputCommandsBuffer.GetBuffer(),
                {},
                mCullerInputCommandsBuffer.GetSize()
            ),
            vk::BufferMemoryBarrier2( // scene
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eFragmentShader,
                vk::AccessFlagBits2::eShaderStorageRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mSceneBuffer.GetBuffer(),
                {},
                mSceneBuffer.GetSize()
            )
        };
    }

    vk::BufferMemoryBarrier2 FrameData::GetPickerBufferMemoryBarrier() const
    {
        return {
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eShaderWrite,
            vk::PipelineStageFlagBits2::eHost,
            vk::AccessFlagBits2::eHostRead,
            vk::QueueFamilyIgnored,
            vk::QueueFamilyIgnored,
            mPickerBuffer.GetBuffer(),
            {},
            sizeof(uint32_t)
        };
    }

    std::array<vk::BufferMemoryBarrier2, FrameData::kCullerBufferMemoryBarriersCount> FrameData::
    GetCullerBufferMemoryBarriers() const
    {
        return {
            vk::BufferMemoryBarrier2( // indirect buffer
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eDrawIndirect,
                vk::AccessFlagBits2::eIndirectCommandRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mIndirectBuffer.GetBuffer(),
                {},
                mIndirectBuffer.GetSize()
            ),
            vk::BufferMemoryBarrier2( // culler count
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eDrawIndirect,
                vk::AccessFlagBits2::eIndirectCommandRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mCullerCountBuffer.GetBuffer(),
                {},
                mCullerCountBuffer.GetSize()
            )
        };
    }

    vk::BufferMemoryBarrier2 FrameData::GetCullerCountBufferFillMemoryBarrier() const
    {
        return {
            vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eTransferWrite,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite,
            vk::QueueFamilyIgnored,
            vk::QueueFamilyIgnored,
            mCullerCountBuffer.GetBuffer(),
            {},
            mCullerCountBuffer.GetSize()
        };
    }

    std::array<vk::BufferMemoryBarrier2, FrameData::kIndirectReadToWriteMemoryBarriersCount> FrameData::GetIndirectReadToWriteBarriers() const
    {
        return {
            vk::BufferMemoryBarrier2{ // indirect buffer
                vk::PipelineStageFlagBits2::eDrawIndirect, vk::AccessFlagBits2::eIndirectCommandRead,
                vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                mIndirectBuffer.GetBuffer(), 0, vk::WholeSize
            },
            vk::BufferMemoryBarrier2{  // culler count
                vk::PipelineStageFlagBits2::eDrawIndirect, vk::AccessFlagBits2::eIndirectCommandRead,
                vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                mCullerCountBuffer.GetBuffer(), 0, vk::WholeSize
            }
        };
    }

    void FrameData::CreateCommandPool(const Context &context)
    {
        vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        mCommandPool = vk::raii::CommandPool(context.mDevice, poolInfo);
    }

    void FrameData::CreateImGuiCommandPool(const Context &context)
    {
        vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        mImGuiCommandPool = vk::raii::CommandPool(context.mDevice, poolInfo);
    }

    void FrameData::CreateCommandBuffer(const Context &context)
    {
        vk::CommandBufferAllocateInfo allocInfo(
            mCommandPool,
            vk::CommandBufferLevel::ePrimary,
            1
        );

        mCommandBuffer = std::move(vk::raii::CommandBuffers(context.mDevice, allocInfo).front());
    }

    void FrameData::CreateImGuiCommandBuffer(const Context &context)
    {
        vk::CommandBufferAllocateInfo allocInfo(
            mCommandPool,
            vk::CommandBufferLevel::eSecondary,
            1
        );

        mImGuiCommandBuffer = std::move(vk::raii::CommandBuffers(context.mDevice, allocInfo).front());
    }

    void FrameData::CreateSyncObjects(const Context &context)
    {
        mFenceInFlight = vk::raii::Fence(context.mDevice, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }

    void FrameData::CreateMeshDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                        const DescriptorPool &descriptorPool, std::span<const DescriptorSetInfo> infos)
    {
        mMeshDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::CreateSkyboxDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                              const DescriptorPool &descriptorPool,
                                              std::span<const DescriptorSetInfo> infos)
    {
        mSkyboxDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::CreateGizmoDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
        const DescriptorPool &descriptorPool, std::span<const DescriptorSetInfo> infos)
    {
        mGizmoDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::CreatePickerDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                              const DescriptorPool &descriptorPool,
                                              std::span<const DescriptorSetInfo> infos)
    {
        mPickerDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::CreateOutlineDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                               const DescriptorPool &descriptorPool,
                                               std::span<const DescriptorSetInfo> infos)
    {
        mOutlineDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::CreateCullerDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                              const DescriptorPool &descriptorPool,
                                              std::span<const DescriptorSetInfo> infos)
    {
        mCullerDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::CreateCameraBuffer(const Context &context)
    {
        mCameraBuffer = BufferAllocator::alloc_uniform(context, sizeof(CameraData) * details::kMaxCameras);
    }

    void FrameData::CreateMeshDataBuffer(const Context &context)
    {
        mMeshDataBuffer = BufferAllocator::alloc_storage(context, details::kMaxMeshes * sizeof(MeshData));
    }

    void FrameData::CreateMaterialsBuffer(const Context &context)
    {
        mMaterialsBuffer = BufferAllocator::alloc_storage(context, details::kMaxMaterials * sizeof(MaterialSlot));
    }

    void FrameData::CreateIndirectBuffer(const Context &context)
    {
        mIndirectBuffer = BufferAllocator::alloc_host(context, details::kMaxMeshes * sizeof(vk::DrawIndexedIndirectCommand),
                                                       vk::BufferUsageFlagBits::eIndirectBuffer |
                                                       vk::BufferUsageFlagBits::eStorageBuffer);
    }

    void FrameData::CreateSceneBuffer(const Context &context)
    {
        mSceneBuffer = BufferAllocator::alloc_storage(context, sizeof(SceneData));
    }

    void FrameData::CreatePickerBuffer(const Context &context)
    {
        mPickerBuffer = BufferAllocator::alloc_storage(context, sizeof(uint32_t));
    }

    void FrameData::CreateCullerBuffers(const Context &context)
    {
        mCullerInputCommandsBuffer = BufferAllocator::alloc_host(context, details::kMaxMeshes * sizeof(vk::DrawIndexedIndirectCommand),
                                                                  vk::BufferUsageFlagBits::eStorageBuffer);
        mCullerCountBuffer = BufferAllocator::alloc_local(context, sizeof(uint32_t),
                                                           vk::BufferUsageFlagBits::eStorageBuffer |
                                                           vk::BufferUsageFlagBits::eIndirectBuffer);
    }

    void FrameData::CreateSceneTexture(const Context &context, vk::Format format)
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

    void FrameData::CreateOutIdTexture(const Context &context)
    {
        mOutIdTexture = TextureAllocator::create_empty(
            context,
            mExtent.width,
            mExtent.height,
            vk::Format::eR32Uint,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::ImageAspectFlagBits::eColor,
            context.GetMaxUsableSampleCount()
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

    std::array<DescriptorSetInfo, FrameData::kDescriptorSetInfoCount> FrameData::MakeMeshDescriptorSetInfo(
        const SkyboxPass &skybox, const TextureRegistry &textureRegistry) const
    {
        return {
            DescriptorSetBufferInfo(
                mCameraBuffer.GetBuffer(),
                mCameraBuffer.GetSize(),
                1,
                vk::DescriptorType::eUniformBuffer
            ),
            DescriptorSetBufferInfo(
                mMeshDataBuffer.GetBuffer(),
                mMeshDataBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                mMaterialsBuffer.GetBuffer(),
                mMaterialsBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                mSceneBuffer.GetBuffer(),
                mSceneBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetImageInfo(
                skybox.GetTexture().GetSampler(),
                skybox.GetTexture().GetImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1
            ),
            DescriptorSetImageInfo(
                skybox.GetIrradianceMapTexture().GetSampler(),
                skybox.GetIrradianceMapTexture().GetImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1
            ),
            DescriptorSetImageInfo(
                skybox.GetPrefilteredEnvTexture().GetSampler(),
                skybox.GetPrefilteredEnvTexture().GetImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1
            ),
            DescriptorSetImageInfo(
                skybox.GetBrdfLutTexture().GetSampler(),
                skybox.GetBrdfLutTexture().GetImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1
            ),
            DescriptorSetImageInfo(
                textureRegistry.GetTexture(textureRegistry.GetDefaultTextureHandle(TextureType::Albedo)).GetSampler(),
                textureRegistry.GetTexture(textureRegistry.GetDefaultTextureHandle(TextureType::Albedo)).GetImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                details::kMaxTextures
            )
        };
    }

    std::array<DescriptorSetInfo, FrameData::kSkyboxDescriptorSetInfoCount> FrameData::MakeSkyboxDescriptorSetInfo(
        const Texture &skyboxTexture) const
    {
        return {
            DescriptorSetBufferInfo(
                mCameraBuffer.GetBuffer(),
                mCameraBuffer.GetSize(),
                1,
                vk::DescriptorType::eUniformBuffer
            ),
            DescriptorSetImageInfo(
                skyboxTexture.GetSampler(),
                skyboxTexture.GetImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1
            )
        };
    }

    std::array<DescriptorSetInfo, FrameData::kGizmoDescriptorSetInfoCount> FrameData::MakeGizmoDescriptorSetInfo() const
    {
        return {
            DescriptorSetBufferInfo(
                mCameraBuffer.GetBuffer(),
                mCameraBuffer.GetSize(),
                1,
                vk::DescriptorType::eUniformBuffer
                )
        };
    }

    std::array<DescriptorSetInfo, FrameData::kPickerDescriptorSetInfoCount>
    FrameData::MakePickerDescriptorSetInfo() const
    {
        return {
            DescriptorSetImageInfo(
                nullptr,
                mResolvedOutIdTexture.GetImageView(),
                vk::ImageLayout::eGeneral,
                1,
                vk::DescriptorType::eStorageImage
            ),
            DescriptorSetBufferInfo(
                mPickerBuffer.GetBuffer(),
                mPickerBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            )
        };
    }

    std::array<DescriptorSetInfo, FrameData::kOutlineDescriptorSetInfoCount> FrameData::
    MakeOutlineDescriptorSetInfo() const
    {
        return {
            DescriptorSetImageInfo(
                mResolvedOutIdTexture.GetSampler(),
                mResolvedOutIdTexture.GetImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1,
                vk::DescriptorType::eCombinedImageSampler
            )
        };
    }

    std::array<DescriptorSetInfo, FrameData::kCullerDescriptorSetInfoCount> FrameData::
    MakeCullerDescriptorSetInfo() const
    {
        return {
            DescriptorSetBufferInfo(
                mMeshDataBuffer.GetBuffer(),
                mMeshDataBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                mCullerInputCommandsBuffer.GetBuffer(),
                mCullerInputCommandsBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                mIndirectBuffer.GetBuffer(),
                mIndirectBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                mCullerCountBuffer.GetBuffer(),
                mCullerCountBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            )
        };
    }
}
