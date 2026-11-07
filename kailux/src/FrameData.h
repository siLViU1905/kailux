#pragma once
#include "Context.h"
#include "RenderTarget.h"
#include "passes/SkyboxPass.h"
#include "buffer/Buffer.h"
#include "command/CommandRecorder.h"
#include "descriptor/DescriptorSet.h"
#include "passes/ComputeCuller.h"
#include "passes/ComputeCompactor.h"
#include "passes/ComputeShadowCuller.h"
#include "mesh/DrawBatches.h"
#include "texture/Texture.h"
#include "texture/TextureRegistry.h"
#include "passes/ComputePicker.h"
#include "passes/GizmoPass.h"
#include "passes/MainPass.h"
#include "passes/OutlinePass.h"
#include "passes/ShadowPass.h"
#include "shadow/ShadowMap.h"

namespace kailux
{
    struct MeshUploadState
    {
        DrawBatches batches;
        uint32_t    objectCount{};

        uint64_t materialGeneration{~0ull};

        std::array<uint32_t, details::kPointShadowDrawLists> pointShadowDrawCounts{};
    };

    class FrameData
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(FrameData)

        static FrameData create(const Context &context,
                                const Swapchain &swapchain,
                                const RenderTarget &sceneView,
                                const MainPass & mainPass,
                                const SkyboxPass &skybox,
                                const GizmoPass & gizmoPass,
                                const ComputePicker& picker,
                                const OutlinePass& outlinePass,
                                const ComputeCuller& culler,
                                const ComputeCompactor& compactor,
                                const ComputeShadowCuller& shadowCuller,
                                const ShadowPass& shadowPass,
                                const ShadowMap& directionalShadowMap,
                                const ShadowMap& pointShadowMap,
                                const TextureRegistry &textureRegistry
        );

        void Reset(const Context& context) const;

        void RebindSceneViewTextures(const Context &context, const RenderTarget &target);

        vk::CommandBuffer GetCommandBuffer() const;
        vk::CommandBuffer GetImGuiCommandBuffer() const;
        vk::Fence         GetFenceInFlight() const;

        const DescriptorSet& GetMeshDescriptorSet() const;
        const DescriptorSet& GetSkyboxDescriptorSet() const;
        const DescriptorSet& GetGizmoDescriptorSet() const;
        const DescriptorSet& GetPickerDescriptorSet() const;
        const DescriptorSet& GetOutlineDescriptorSet() const;
        const DescriptorSet& GetCullerDescriptorSet() const;
        const DescriptorSet& GetCompactorDescriptorSet() const;
        const DescriptorSet& GetShadowDescriptorSet() const;
        const DescriptorSet& GetShadowCullerDescriptorSet() const;

        Buffer&       GetCameraBuffer();
        Buffer&       GetModelBuffer();
        Buffer&       GetMaterialBuffer();
        Buffer&       GetIndirectBuffer();
        const Buffer& GetIndirectBuffer() const;
        Buffer&       GetSceneBuffer();
        const Buffer& GetPickerBuffer() const;
        const Buffer& GetCullerInputCommandsBuffer() const;
        const Buffer& GetBatchCommandsBuffer() const;
        const Buffer& GetInstanceIdsBuffer() const;
        const Buffer& GetCullerCountBuffer() const;
        const Buffer& GetDirectionalShadowCommandsBuffer() const;
        const Buffer& GetDirectionalShadowTemplatesBuffer() const;
        const Buffer& GetShadowCullTargetsBuffer() const;
        const Buffer& GetPointShadowCommandsBuffer() const;

        decltype(auto) GetMeshUploadState(this auto&& self)
        {
            return (self.mMeshUploadState);
        }

        static constexpr uint32_t kBufferMemoryBarriersCount{1 + 1 + 1 + 1 + 1 + 1 + 1 + 1}; // camera buffer + mesh data buffer + materials buffer + culler input buffer + directional shadow templates + shadow cull targets + point shadow commands + scene buffer
        std::array<vk::BufferMemoryBarrier2, kBufferMemoryBarriersCount>       GetBufferMemoryBarriers() const;

        vk::BufferMemoryBarrier2                                               GetPickerBufferMemoryBarrier() const;

        static constexpr uint32_t kCullerBufferMemoryBarriersCount{1 + 1}; // indirect buffer + culler count buffer
        std::array<vk::BufferMemoryBarrier2, kCullerBufferMemoryBarriersCount> GetCullerBufferMemoryBarriers() const;

        static constexpr uint32_t kCullerFillMemoryBarriersCount{1 + 1}; // culler count buffer + batch commands
        std::array<vk::BufferMemoryBarrier2, kCullerFillMemoryBarriersCount> GetCullerFillMemoryBarriers() const;

        static constexpr uint32_t kCullerOutputMemoryBarriersCount{1 + 1}; // batch commands + instance ids
        std::array<vk::BufferMemoryBarrier2, kCullerOutputMemoryBarriersCount> GetCullerOutputMemoryBarriers() const;

        static constexpr uint32_t kShadowCullerFillMemoryBarriersCount{1}; // directional shadow commands
        std::array<vk::BufferMemoryBarrier2, kShadowCullerFillMemoryBarriersCount> GetShadowCullerFillMemoryBarriers() const;

        static constexpr uint32_t kShadowCullerOutputMemoryBarriersCount{1 + 1}; // directional shadow commands + instance ids
        std::array<vk::BufferMemoryBarrier2, kShadowCullerOutputMemoryBarriersCount> GetShadowCullerOutputMemoryBarriers() const;

        static constexpr uint32_t kIndirectReadToWriteMemoryBarriersCount{1 + 1 + 1 + 1}; // indirect buffer + culler count buffer + batch commands + instance ids
        std::array<vk::BufferMemoryBarrier2, kIndirectReadToWriteMemoryBarriersCount> GetIndirectReadToWriteBarriers() const;

    private:
        void CreateCommandPool(const Context& context);
        //Separate command pool for future imgui separate thread integration
        void CreateImGuiCommandPool(const Context& context);
        void CreateCommandBuffer(const Context& context);
        void CreateImGuiCommandBuffer(const Context& context);
        void CreateSyncObjects(const Context& context);
        void CreateMeshDescriptorSet(const Context& context, const DescriptorLayout& descriptorLayout, const DescriptorPool& descriptorPool, std::span<const
                                 DescriptorSetInfo> infos);
        void CreateSkyboxDescriptorSet(const Context& context, const DescriptorLayout& descriptorLayout, const DescriptorPool& descriptorPool, std::span<const
                                       DescriptorSetInfo> infos);
        void CreateGizmoDescriptorSet(const Context& context, const DescriptorLayout& descriptorLayout, const DescriptorPool& descriptorPool, std::span<const
                                       DescriptorSetInfo> infos);
        void CreatePickerDescriptorSet(const Context& context, const DescriptorLayout& descriptorLayout, const DescriptorPool& descriptorPool, std::span<const
                                       DescriptorSetInfo> infos);
        void CreateOutlineDescriptorSet(const Context& context, const DescriptorLayout& descriptorLayout, const DescriptorPool& descriptorPool, std::span<const
                                       DescriptorSetInfo> infos);
        void CreateCullerDescriptorSet(const Context& context, const DescriptorLayout& descriptorLayout, const DescriptorPool& descriptorPool, std::span<const
                                       DescriptorSetInfo> infos);
        void CreateCompactorDescriptorSet(const Context& context, const DescriptorLayout& descriptorLayout, const DescriptorPool& descriptorPool, std::span<const
                                       DescriptorSetInfo> infos);
        void CreateShadowDescriptorSet(const Context& context, const DescriptorLayout& descriptorLayout, const DescriptorPool& descriptorPool, std::span<const
                                       DescriptorSetInfo> infos);
        void CreateShadowCullerDescriptorSet(const Context& context, const DescriptorLayout& descriptorLayout, const DescriptorPool& descriptorPool, std::span<const
                                       DescriptorSetInfo> infos);
        void CreateCameraBuffer(const Context& context);
        void CreateMeshDataBuffer(const Context &context);
        void CreateMaterialsBuffer(const Context &context);
        void CreateIndirectBuffer(const Context &context);
        void CreateSceneBuffer(const Context& context);
        void CreatePickerBuffer(const Context& context);
        void CreateCullerBuffers(const Context &context);
        void CreateDirectionalShadowBuffer(const Context &context);
        void CreatePointShadowBuffer(const Context &context);

        void SeedPointShadowDescriptors(const Context &context, const ShadowMap& pointShadowMap);

        // camera buffer + mesh data buffer +
        // materials buffer + scene buffer +
        // skybox sampler + irradiance map +
        // prefiltered env + brdf lut +
        // directional shadow + point shadow cubes +
        // textures array + instance ids
        static constexpr uint32_t kDescriptorSetInfoCount{12};
        static constexpr uint32_t kSkyboxDescriptorSetInfoCount{1 + 1}; // camera buffer + cube texture
        static constexpr uint32_t kGizmoDescriptorSetInfoCount{1}; // camera buffer
        static constexpr uint32_t kPickerDescriptorSetInfoCount{1 + 1}; // id image + out buffer
        static constexpr uint32_t kOutlineDescriptorSetInfoCount{1}; // id image
        static constexpr uint32_t kCullerDescriptorSetInfoCount{3}; // mesh data + batch commands + out instance ids
        static constexpr uint32_t kCompactorDescriptorSetInfoCount{3}; // batch commands + out indirect + counter
        static constexpr uint32_t kShadowDescriptorSetInfoCount{1 + 1}; // mesh data + directional shadow instance ids
        static constexpr uint32_t kShadowCullerDescriptorSetInfoCount{4}; // mesh data + targets + shadow commands + out instance ids
                std::array<DescriptorSetInfo, kDescriptorSetInfoCount> MakeMeshDescriptorSetInfo(
            const SkyboxPass &skybox,
            const TextureRegistry &textureRegistry,
            const ShadowMap &directionalShadowMap,
            const ShadowMap &pointShadowMap
        ) const;
        std::array<DescriptorSetInfo, kSkyboxDescriptorSetInfoCount>  MakeSkyboxDescriptorSetInfo(const Texture& skyboxTexture) const;
        std::array<DescriptorSetInfo, kGizmoDescriptorSetInfoCount>   MakeGizmoDescriptorSetInfo() const;
        std::array<DescriptorSetInfo, kPickerDescriptorSetInfoCount>  MakePickerDescriptorSetInfo(const RenderTarget &sceneView) const;
        std::array<DescriptorSetInfo, kOutlineDescriptorSetInfoCount> MakeOutlineDescriptorSetInfo(const RenderTarget &sceneView) const;
        std::array<DescriptorSetInfo, kCullerDescriptorSetInfoCount>  MakeCullerDescriptorSetInfo() const;
        std::array<DescriptorSetInfo, kCompactorDescriptorSetInfoCount> MakeCompactorDescriptorSetInfo() const;
        std::array<DescriptorSetInfo, kShadowDescriptorSetInfoCount>  MakeShadowDescriptorSetInfo() const;
        std::array<DescriptorSetInfo, kShadowCullerDescriptorSetInfoCount> MakeShadowCullerDescriptorSetInfo() const;
        static constexpr uint32_t kPickerResolvedViewDescriptorSetBinding = 0;
        static constexpr uint32_t kOutlineIdResolvedViewDescriptorSetBinding = 0;

        vk::raii::CommandPool   mCommandPool;
        vk::raii::CommandPool   mImGuiCommandPool;
        vk::raii::CommandBuffer mCommandBuffer;
        vk::raii::CommandBuffer mImGuiCommandBuffer;
        vk::raii::Fence         mFenceInFlight;

        DescriptorSet           mMeshDescriptorSet;
        DescriptorSet           mSkyboxDescriptorSet;
        DescriptorSet           mGizmoDescriptorSet;
        DescriptorSet           mPickerDescriptorSet;
        DescriptorSet           mOutlineDescriptorSet;
        DescriptorSet           mCullerDescriptorSet;
        DescriptorSet           mCompactorDescriptorSet;
        DescriptorSet           mShadowDescriptorSet;
        DescriptorSet           mShadowCullerDescriptorSet;

        Buffer                  mCameraBuffer;
        Buffer                  mMeshDataBuffer;
        Buffer                  mMaterialsBuffer;
        Buffer                  mIndirectBuffer;
        Buffer                  mSceneBuffer;
        Buffer                  mPickerBuffer;
        Buffer                  mCullerInputCommandsBuffer;
        Buffer                  mCullerCountBuffer;
        Buffer                  mBatchCommandsBuffer;
        Buffer                  mInstanceIdsBuffer;
        Buffer                  mDirectionalShadowCommandsBuffer;
        Buffer                  mDirectionalShadowTemplatesBuffer;
        Buffer                  mDirectionalShadowInstanceIdsBuffer;
        Buffer                  mShadowCullTargetsBuffer;
        Buffer                  mPointShadowCommandsBuffer;

        MeshUploadState         mMeshUploadState;
    };
}
