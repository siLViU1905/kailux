#include "FrameData.h"

#include "pipeline/Pipeline.h"
#include "buffer/BufferAllocator.h"
#include "components/gpu/CameraData.h"
#include "components/gpu/MeshData.h"
#include "components/gpu/SceneData.h"
#include  "texture/TextureAllocator.h"
#include "profiling/Profiling.h"

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
                                                       mCompactorDescriptorSet(std::move(other.mCompactorDescriptorSet)),                                                       mShadowDescriptorSet(std::move(other.mShadowDescriptorSet)),
                                                       mShadowCullerDescriptorSet(std::move(other.mShadowCullerDescriptorSet)),                                                       mCameraBuffer(std::move(other.mCameraBuffer)),
                                                       mMeshDataBuffer(std::move(other.mMeshDataBuffer)),
                                                       mMaterialsBuffer(std::move(other.mMaterialsBuffer)),
                                                       mIndirectBuffer(std::move(other.mIndirectBuffer)),
                                                       mSceneBuffer(std::move(other.mSceneBuffer)),
                                                       mPickerBuffer(std::move(other.mPickerBuffer)),
                                                       mCullerInputCommandsBuffer(
                                                           std::move(other.mCullerInputCommandsBuffer)),
                                                       mCullerCountBuffer(std::move(other.mCullerCountBuffer)),
                                                       mBatchCommandsBuffer(std::move(other.mBatchCommandsBuffer)),
                                                       mInstanceIdsBuffer(std::move(other.mInstanceIdsBuffer)),
                                                       mDirectionalShadowCommandsBuffer(
                                                           std::move(other.mDirectionalShadowCommandsBuffer)),
                                                       mDirectionalShadowTemplatesBuffer(
                                                           std::move(other.mDirectionalShadowTemplatesBuffer)),
                                                       mDirectionalShadowInstanceIdsBuffer(
                                                           std::move(other.mDirectionalShadowInstanceIdsBuffer)),
                                                       mShadowCullTargetsBuffer(std::move(other.mShadowCullTargetsBuffer)),
                                                       mPointShadowCommandsBuffer(
                                                           std::move(other.mPointShadowCommandsBuffer)),
                                                       mMeshUploadState(std::move(other.mMeshUploadState))
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
            mCompactorDescriptorSet = std::move(other.mCompactorDescriptorSet);
            mShadowDescriptorSet = std::move(other.mShadowDescriptorSet);
            mShadowCullerDescriptorSet = std::move(other.mShadowCullerDescriptorSet);
            mCameraBuffer = std::move(other.mCameraBuffer);
            mMeshDataBuffer = std::move(other.mMeshDataBuffer);
            mMaterialsBuffer = std::move(other.mMaterialsBuffer);
            mIndirectBuffer = std::move(other.mIndirectBuffer);
            mSceneBuffer = std::move(other.mSceneBuffer);
            mPickerBuffer = std::move(other.mPickerBuffer);
            mCullerInputCommandsBuffer = std::move(other.mCullerInputCommandsBuffer);
            mCullerCountBuffer = std::move(other.mCullerCountBuffer);
            mBatchCommandsBuffer = std::move(other.mBatchCommandsBuffer);
            mInstanceIdsBuffer = std::move(other.mInstanceIdsBuffer);
            mDirectionalShadowCommandsBuffer = std::move(other.mDirectionalShadowCommandsBuffer);
            mDirectionalShadowTemplatesBuffer = std::move(other.mDirectionalShadowTemplatesBuffer);
            mDirectionalShadowInstanceIdsBuffer = std::move(other.mDirectionalShadowInstanceIdsBuffer);
            mShadowCullTargetsBuffer = std::move(other.mShadowCullTargetsBuffer);
            mPointShadowCommandsBuffer = std::move(other.mPointShadowCommandsBuffer);
            mMeshUploadState = std::move(other.mMeshUploadState);
        }
        return *this;
    }

    FrameData FrameData::create(
        const Context &context,
        const Swapchain &swapchain,
        const RenderTarget &sceneView,
        const MainPass &mainPass,
        const SkyboxPass &skybox,
        const GizmoPass & gizmoPass,
        const ComputePicker &picker,
        const OutlinePass &outlinePass,
        const ComputeCuller &culler,
        const ComputeCompactor& compactor,
        const ComputeShadowCuller& shadowCuller,
        const ShadowPass& shadowPass,
        const ShadowMap& directionalShadowMap,
        const ShadowMap& pointShadowMap,
        const TextureRegistry &textureRegistry
    )
    {
        FrameData frame;
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
        frame.CreateDirectionalShadowBuffer(context);
        frame.CreatePointShadowBuffer(context);
        const auto descSetInfo{frame.MakeMeshDescriptorSetInfo(skybox, textureRegistry, directionalShadowMap, pointShadowMap)};
        frame.CreateMeshDescriptorSet(context, mainPass.GetDescriptorLayout(), mainPass.GetDescriptorPool(), descSetInfo);
        const auto skyboxDescInfo{frame.MakeSkyboxDescriptorSetInfo(skybox.GetTexture())};
        frame.CreateSkyboxDescriptorSet(context, skybox.GetDescriptorLayout(), skybox.GetDescriptorPool(),
                                        skyboxDescInfo);
        const auto gizmoDescInfo{frame.MakeGizmoDescriptorSetInfo()};
        frame.CreateGizmoDescriptorSet(context, gizmoPass.GetDescriptorLayout(), gizmoPass.GetDescriptorPool(), gizmoDescInfo);
        const auto pickerDescInfo{frame.MakePickerDescriptorSetInfo(sceneView)};
        frame.CreatePickerDescriptorSet(context, picker.GetDescriptorLayout(), picker.GetDescriptorPool(),
                                        pickerDescInfo);
        const auto outlineDescInfo{frame.MakeOutlineDescriptorSetInfo(sceneView)};
        frame.CreateOutlineDescriptorSet(context, outlinePass.GetDescriptorLayout(), outlinePass.GetDescriptorPool(),
                                         outlineDescInfo);
        const auto cullerDescInfo{frame.MakeCullerDescriptorSetInfo()};
        frame.CreateCullerDescriptorSet(context, culler.GetDescriptorLayout(), culler.GetDescriptorPool(),
                                        cullerDescInfo);
        const auto compactorDescInfo{frame.MakeCompactorDescriptorSetInfo()};
        frame.CreateCompactorDescriptorSet(context, compactor.GetDescriptorLayout(), compactor.GetDescriptorPool(),
                                           compactorDescInfo);
        const auto shadowDescInfo{frame.MakeShadowDescriptorSetInfo()};
        frame.CreateShadowDescriptorSet(context, shadowPass.GetDescriptorLayout(), shadowPass.GetDescriptorPool(),
                                        shadowDescInfo);
        const auto shadowCullerDescInfo{frame.MakeShadowCullerDescriptorSetInfo()};
        frame.CreateShadowCullerDescriptorSet(context, shadowCuller.GetDescriptorLayout(), shadowCuller.GetDescriptorPool(),
                                              shadowCullerDescInfo);
        frame.SeedPointShadowDescriptors(context, pointShadowMap);
        return frame;
    }

    void FrameData::Reset(const Context &context) const
    {
        KAILUX_PROFILE_CPU(CpuSection::FrameWait);
        auto result = context.GetDevice().waitForFences(*mFenceInFlight, true, UINT64_MAX);
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("waitForFences failed");

        context.GetDevice().resetFences(*mFenceInFlight);

        mCommandPool.reset();
    }

    void FrameData::RebindSceneViewTextures(const Context &context, const RenderTarget &target)
    {
        if (!target.HasIdTexture())
            return;

        const auto &idTexture{target.GetReadableIdTexture()};

        const std::array<DescriptorSetUpdateInfo, 1> pickerInfo{
            DescriptorSetUpdateInfo(kPickerResolvedViewDescriptorSetBinding,
                                    0,
                                    DescriptorSetImageInfo(
                                        nullptr,
                                        idTexture.GetImageView(),
                                        vk::ImageLayout::eGeneral,
                                        1,
                                        vk::DescriptorType::eStorageImage
                                    ))
        };
        mPickerDescriptorSet.UpdateInfo(context, pickerInfo);

        const std::array<DescriptorSetUpdateInfo, 1> outlineInfo{
            DescriptorSetUpdateInfo(kOutlineIdResolvedViewDescriptorSetBinding,
                                    0,
                                    DescriptorSetImageInfo(
                                        idTexture.GetSampler(),
                                        idTexture.GetImageView(),
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

    const DescriptorSet & FrameData::GetCompactorDescriptorSet() const
    {
        return mCompactorDescriptorSet;
    }

    const DescriptorSet & FrameData::GetShadowDescriptorSet() const
    {
        return mShadowDescriptorSet;
    }

    const DescriptorSet &FrameData::GetShadowCullerDescriptorSet() const
    {
        return mShadowCullerDescriptorSet;
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

    const Buffer & FrameData::GetBatchCommandsBuffer() const
    {
        return mBatchCommandsBuffer;
    }

    const Buffer & FrameData::GetInstanceIdsBuffer() const
    {
        return mInstanceIdsBuffer;
    }

    const Buffer &FrameData::GetCullerCountBuffer() const
    {
        return mCullerCountBuffer;
    }

    const Buffer & FrameData::GetDirectionalShadowCommandsBuffer() const
    {
        return mDirectionalShadowCommandsBuffer;
    }

    const Buffer &FrameData::GetDirectionalShadowTemplatesBuffer() const
    {
        return mDirectionalShadowTemplatesBuffer;
    }

    const Buffer &FrameData::GetShadowCullTargetsBuffer() const
    {
        return mShadowCullTargetsBuffer;
    }

    const Buffer & FrameData::GetPointShadowCommandsBuffer() const
    {
        return mPointShadowCommandsBuffer;
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
            vk::BufferMemoryBarrier2( // culler input (batch command templates, copied at the start of culling)
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eTransfer,
                vk::AccessFlagBits2::eTransferRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mCullerInputCommandsBuffer.GetBuffer(),
                {},
                mCullerInputCommandsBuffer.GetSize()
            ),
            vk::BufferMemoryBarrier2( // directional shadows commands
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eTransfer,
                vk::AccessFlagBits2::eTransferRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mDirectionalShadowTemplatesBuffer.GetBuffer(),
                {},
                mDirectionalShadowTemplatesBuffer.GetSize()
            ),
            vk::BufferMemoryBarrier2( // shadow cull targets
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderStorageRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mShadowCullTargetsBuffer.GetBuffer(),
                {},
                mShadowCullTargetsBuffer.GetSize()
            ),
            vk::BufferMemoryBarrier2( // point shadows commands
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostWrite,
                vk::PipelineStageFlagBits2::eDrawIndirect,
                vk::AccessFlagBits2::eIndirectCommandRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mPointShadowCommandsBuffer.GetBuffer(),
                {},
                mPointShadowCommandsBuffer.GetSize()
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

    std::array<vk::BufferMemoryBarrier2, FrameData::kCullerFillMemoryBarriersCount> FrameData::
    GetCullerFillMemoryBarriers() const
    {
        return {
            vk::BufferMemoryBarrier2( // culler count, cleared -> compactor atomics
                vk::PipelineStageFlagBits2::eTransfer,
                vk::AccessFlagBits2::eTransferWrite,
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mCullerCountBuffer.GetBuffer(),
                {},
                mCullerCountBuffer.GetSize()
            ),
            vk::BufferMemoryBarrier2( // batch commands, copied from the templates -> culler atomics
                vk::PipelineStageFlagBits2::eTransfer,
                vk::AccessFlagBits2::eTransferWrite,
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mBatchCommandsBuffer.GetBuffer(),
                {},
                mBatchCommandsBuffer.GetSize()
            )
        };
    }

    std::array<vk::BufferMemoryBarrier2, FrameData::kCullerOutputMemoryBarriersCount> FrameData::
    GetCullerOutputMemoryBarriers() const
    {
        return {
            vk::BufferMemoryBarrier2( // batch commands -> compactor
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mBatchCommandsBuffer.GetBuffer(),
                {},
                mBatchCommandsBuffer.GetSize()
            ),
            vk::BufferMemoryBarrier2( // instance ids -> main pass vertex shader
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eVertexShader,
                vk::AccessFlagBits2::eShaderStorageRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mInstanceIdsBuffer.GetBuffer(),
                {},
                mInstanceIdsBuffer.GetSize()
            )
        };
    }

    std::array<vk::BufferMemoryBarrier2, FrameData::kShadowCullerFillMemoryBarriersCount> FrameData::
    GetShadowCullerFillMemoryBarriers() const
    {
        return {
            vk::BufferMemoryBarrier2( // directional shadow commands, copied from the templates -> shadow culler atomics
                vk::PipelineStageFlagBits2::eTransfer,
                vk::AccessFlagBits2::eTransferWrite,
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mDirectionalShadowCommandsBuffer.GetBuffer(),
                {},
                mDirectionalShadowCommandsBuffer.GetSize()
            )
        };
    }

    std::array<vk::BufferMemoryBarrier2, FrameData::kShadowCullerOutputMemoryBarriersCount> FrameData::
    GetShadowCullerOutputMemoryBarriers() const
    {
        return {
            vk::BufferMemoryBarrier2( // directional shadow commands -> draw indirect
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eDrawIndirect,
                vk::AccessFlagBits2::eIndirectCommandRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mDirectionalShadowCommandsBuffer.GetBuffer(),
                {},
                mDirectionalShadowCommandsBuffer.GetSize()
            ),
            vk::BufferMemoryBarrier2( // directional shadow instance ids -> shadow vertex shader
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eVertexShader,
                vk::AccessFlagBits2::eShaderStorageRead,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                mDirectionalShadowInstanceIdsBuffer.GetBuffer(),
                {},
                mDirectionalShadowInstanceIdsBuffer.GetSize()
            )
        };
    }

    std::array<vk::BufferMemoryBarrier2, FrameData::kIndirectReadToWriteMemoryBarriersCount> FrameData::GetIndirectReadToWriteBarriers() const
    {
        return {
            vk::BufferMemoryBarrier2{ // indirect buffer, rewritten by the compactor
                vk::PipelineStageFlagBits2::eDrawIndirect, vk::AccessFlagBits2::eIndirectCommandRead,
                vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderWrite,
                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                mIndirectBuffer.GetBuffer(), 0, vk::WholeSize
            },
            vk::BufferMemoryBarrier2{  // culler count, cleared with a fill
                vk::PipelineStageFlagBits2::eDrawIndirect, vk::AccessFlagBits2::eIndirectCommandRead,
                vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                mCullerCountBuffer.GetBuffer(), 0, vk::WholeSize
            },
            vk::BufferMemoryBarrier2{  // batch commands, read by the compactor, overwritten by the template copy
                vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderRead,
                vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                mBatchCommandsBuffer.GetBuffer(), 0, vk::WholeSize
            },
            vk::BufferMemoryBarrier2{  // instance ids, read by the main pass, rewritten by the culler
                vk::PipelineStageFlagBits2::eVertexShader, vk::AccessFlagBits2::eShaderStorageRead,
                vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderWrite,
                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                mInstanceIdsBuffer.GetBuffer(), 0, vk::WholeSize
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

    void FrameData::CreateCompactorDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
        const DescriptorPool &descriptorPool, std::span<const DescriptorSetInfo> infos)
    {
        mCompactorDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::CreateShadowDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                              const DescriptorPool &descriptorPool, std::span<const DescriptorSetInfo> infos)
    {
        mShadowDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::CreateShadowCullerDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                                    const DescriptorPool &descriptorPool,
                                                    std::span<const DescriptorSetInfo> infos)
    {
        mShadowCullerDescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::CreateCameraBuffer(const Context &context)
    {
        mCameraBuffer = buffer_allocator::alloc_uniform(context, sizeof(CameraData) * (details::kMaxCameras * details::kMaxCameraViews));
    }

    void FrameData::CreateMeshDataBuffer(const Context &context)
    {
        mMeshDataBuffer = buffer_allocator::alloc_storage(context, details::kMaxMeshes * sizeof(MeshData));
    }

    void FrameData::CreateMaterialsBuffer(const Context &context)
    {
        mMaterialsBuffer = buffer_allocator::alloc_storage(context, details::kMaxMaterials * sizeof(MaterialSlot));
    }

    void FrameData::CreateIndirectBuffer(const Context &context)
    {
        mIndirectBuffer = buffer_allocator::alloc_host(context, details::kMaxMeshes * sizeof(vk::DrawIndexedIndirectCommand),
                                                       vk::BufferUsageFlagBits::eIndirectBuffer |
                                                       vk::BufferUsageFlagBits::eStorageBuffer);
    }

    void FrameData::CreateSceneBuffer(const Context &context)
    {
        mSceneBuffer = buffer_allocator::alloc_storage(context, sizeof(SceneData));
    }

    void FrameData::CreatePickerBuffer(const Context &context)
    {
        mPickerBuffer = buffer_allocator::alloc_storage(context, sizeof(uint32_t));
    }

    void FrameData::CreateCullerBuffers(const Context &context)
    {
        mCullerInputCommandsBuffer = buffer_allocator::alloc_host(
            context,
            details::kMaxBatchCommands * sizeof(vk::DrawIndexedIndirectCommand),
            vk::BufferUsageFlagBits::eTransferSrc);
        mCullerCountBuffer = buffer_allocator::alloc_local(
            context,
            sizeof(uint32_t),
            vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eIndirectBuffer |
            vk::BufferUsageFlagBits::eTransferSrc);
        mBatchCommandsBuffer = buffer_allocator::alloc_local(
            context,
            details::kMaxBatchCommands * sizeof(vk::DrawIndexedIndirectCommand),
            vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eTransferDst);
        mInstanceIdsBuffer = buffer_allocator::alloc_local(
            context,
            details::kMaxInstanceSlots * sizeof(uint32_t),
            vk::BufferUsageFlagBits::eStorageBuffer);
    }

    void FrameData::CreateDirectionalShadowBuffer(const Context &context)
    {
        mDirectionalShadowCommandsBuffer = buffer_allocator::alloc_host(
            context,
            details::kDirectionalShadowDrawLists * details::kMaxMeshes * sizeof(vk::DrawIndexedIndirectCommand),
            vk::BufferUsageFlagBits::eIndirectBuffer |
            vk::BufferUsageFlagBits::eStorageBuffer
            );

        mDirectionalShadowTemplatesBuffer = buffer_allocator::alloc_host(
            context,
            details::kShadowCascadeCount * details::kMaxMeshes * sizeof(vk::DrawIndexedIndirectCommand),
            vk::BufferUsageFlagBits::eTransferSrc
            );

        // kMaxMeshes slots per draw list, every object appears at most once in a list
        mDirectionalShadowInstanceIdsBuffer = buffer_allocator::alloc_local(
            context,
            details::kDirectionalShadowDrawLists * details::kMaxMeshes * sizeof(uint32_t),
            vk::BufferUsageFlagBits::eStorageBuffer
            );

        mShadowCullTargetsBuffer = buffer_allocator::alloc_storage(
            context,
            details::kDirectionalShadowDrawLists * sizeof(ShadowCullTarget)
            );
    }

    void FrameData::CreatePointShadowBuffer(const Context &context)
    {
        mPointShadowCommandsBuffer = buffer_allocator::alloc_host(
            context,
            details::kPointShadowDrawLists * details::kMaxMeshes * sizeof(vk::DrawIndexedIndirectCommand),
            vk::BufferUsageFlagBits::eIndirectBuffer
            );
    }

    void FrameData::SeedPointShadowDescriptors(const Context &context, const ShadowMap& pointShadowMap)
    {
        std::vector<DescriptorSetUpdateInfo> writes;
        writes.reserve(pointShadowMap.GetCubeCount());

        for (uint32_t slot{}; slot < pointShadowMap.GetCubeCount(); ++slot)
            writes.emplace_back(
                MainPass::kMeshPointShadowBindStart,
                slot,
                DescriptorSetImageInfo(
                    pointShadowMap.GetSampler(),
                    pointShadowMap.GetCubeView(slot),
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    1
                )
            );

        mMeshDescriptorSet.UpdateInfo(context, writes);
    }

    std::array<DescriptorSetInfo, FrameData::kDescriptorSetInfoCount> FrameData::MakeMeshDescriptorSetInfo(
        const SkyboxPass &skybox,
        const TextureRegistry &textureRegistry,
        const ShadowMap& directionalShadowMap,
        const ShadowMap& pointShadowMap
        ) const
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
                directionalShadowMap.GetSampler(),
                directionalShadowMap.GetArrayView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1
            ),
            DescriptorSetImageInfo(
                pointShadowMap.GetSampler(),
                pointShadowMap.GetCubeView(0),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                details::kMaxPointShadows * details::kMaxCameraViews
            ),
            DescriptorSetImageInfo(
                textureRegistry.GetTexture(textureRegistry.GetDefaultTextureHandle(TextureType::Albedo)).GetSampler(),
                textureRegistry.GetTexture(textureRegistry.GetDefaultTextureHandle(TextureType::Albedo)).GetImageView(),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                details::kMaxTextures
            ),
            DescriptorSetBufferInfo(
                mInstanceIdsBuffer.GetBuffer(),
                mInstanceIdsBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
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
    FrameData::MakePickerDescriptorSetInfo(const RenderTarget &sceneView) const
    {
        return {
            DescriptorSetImageInfo(
                nullptr,
                sceneView.GetReadableIdTexture().GetImageView(),
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
    MakeOutlineDescriptorSetInfo(const RenderTarget &sceneView) const
    {
        return {
            DescriptorSetImageInfo(
                sceneView.GetReadableIdTexture().GetSampler(),
                sceneView.GetReadableIdTexture().GetImageView(),
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
                mBatchCommandsBuffer.GetBuffer(),
                mBatchCommandsBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                mInstanceIdsBuffer.GetBuffer(),
                mInstanceIdsBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            )
        };
    }

    std::array<DescriptorSetInfo, FrameData::kCompactorDescriptorSetInfoCount> FrameData::
    MakeCompactorDescriptorSetInfo() const
    {
        return {
            DescriptorSetBufferInfo(
                mBatchCommandsBuffer.GetBuffer(),
                mBatchCommandsBuffer.GetSize(),
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

    std::array<DescriptorSetInfo, FrameData::kShadowDescriptorSetInfoCount> FrameData::
    MakeShadowDescriptorSetInfo() const
    {
        return {
            DescriptorSetBufferInfo(
                mMeshDataBuffer.GetBuffer(),
                mMeshDataBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                mDirectionalShadowInstanceIdsBuffer.GetBuffer(),
                mDirectionalShadowInstanceIdsBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            )
        };
    }

    std::array<DescriptorSetInfo, FrameData::kShadowCullerDescriptorSetInfoCount> FrameData::
    MakeShadowCullerDescriptorSetInfo() const
    {
        return {
            DescriptorSetBufferInfo(
                mMeshDataBuffer.GetBuffer(),
                mMeshDataBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                mShadowCullTargetsBuffer.GetBuffer(),
                mShadowCullTargetsBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                mDirectionalShadowCommandsBuffer.GetBuffer(),
                mDirectionalShadowCommandsBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            ),
            DescriptorSetBufferInfo(
                mDirectionalShadowInstanceIdsBuffer.GetBuffer(),
                mDirectionalShadowInstanceIdsBuffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            )
        };
    }
}
