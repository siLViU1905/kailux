#pragma once
#include "Context.h"
#include "passes/SkyboxPass.h"
#include "buffer/Buffer.h"
#include "command/CommandRecorder.h"
#include "descriptor/DescriptorSet.h"
#include "passes/ComputeCuller.h"
#include "texture/Texture.h"
#include "texture/TextureRegistry.h"
#include "passes/ComputePicker.h"
#include "passes/GizmoPass.h"
#include "passes/MainPass.h"
#include "passes/OutlinePass.h"

namespace kailux
{
    class FrameData
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(FrameData)

        static FrameData create(const Context &context,
                                const Swapchain &swapchain,
                                const MainPass & mainPass,
                                const SkyboxPass &skybox,
                                const GizmoPass & gizmoPass,
                                const ComputePicker& picker,
                                const OutlinePass& outlinePass,
                                const ComputeCuller& culler,
                                const TextureRegistry &textureRegistry
        );

        void Reset(const Context& context) const;

        void RecreateTextures(const Context& context, const Swapchain& swapchain);

        vk::CommandBuffer GetCommandBuffer() const;
        vk::CommandBuffer GetImGuiCommandBuffer() const;
        vk::Fence         GetFenceInFlight() const;

        const DescriptorSet& GetMeshDescriptorSet() const;
        const DescriptorSet& GetSkyboxDescriptorSet() const;
        const DescriptorSet& GetGizmoDescriptorSet() const;
        const DescriptorSet& GetPickerDescriptorSet() const;
        const DescriptorSet& GetOutlineDescriptorSet() const;
        const DescriptorSet& GetCullerDescriptorSet() const;

        Buffer&       GetCameraBuffer();
        Buffer&       GetModelBuffer();
        Buffer&       GetMaterialBuffer();
        Buffer&       GetIndirectBuffer();
        const Buffer& GetIndirectBuffer() const;
        Buffer&       GetSceneBuffer();
        const Buffer& GetPickerBuffer() const;
        const Buffer& GetCullerInputCommandsBuffer() const;
        const Buffer& GetCullerCountBuffer() const;

        vk::Extent2D  GetExtent() const;

        const Texture& GetSceneTexture() const;
        const Texture& GetOutIdTexture() const;
        const Texture& GetResolvedOutIdTexture() const;

        static constexpr uint32_t kBufferMemoryBarriersCount{1 + 1 + 1 + 1 + 1}; // camera buffer + mesh data buffer + materials buffer + culler input buffer + scene buffer
        std::array<vk::BufferMemoryBarrier2, kBufferMemoryBarriersCount>       GetBufferMemoryBarriers() const;

        vk::BufferMemoryBarrier2                                               GetPickerBufferMemoryBarrier() const;

        static constexpr uint32_t kCullerBufferMemoryBarriersCount{1 + 1}; // indirect buffer + culler count buffer
        std::array<vk::BufferMemoryBarrier2, kCullerBufferMemoryBarriersCount> GetCullerBufferMemoryBarriers() const;

        vk::BufferMemoryBarrier2                                               GetCullerCountBufferFillMemoryBarrier() const;

        static constexpr uint32_t kIndirectReadToWriteMemoryBarriersCount{1 + 1}; // indirect buffer + culler count buffer
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
        void CreateCameraBuffer(const Context& context);
        void CreateMeshDataBuffer(const Context &context);
        void CreateMaterialsBuffer(const Context &context);
        void CreateIndirectBuffer(const Context &context);
        void CreateSceneBuffer(const Context& context);
        void CreatePickerBuffer(const Context& context);
        void CreateCullerBuffers(const Context &context);

        void CreateSceneTexture(const Context& context, vk::Format format);
        void CreateOutIdTexture(const Context &context);

        static constexpr uint32_t kDescriptorSetInfoCount = 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1; // camera buffer + mesh data buffer + materials buffer + scene buffer + skybox sampler + irradiance map + prefiltered env + brdf lut + textures array
        static constexpr uint32_t kSkyboxDescriptorSetInfoCount = 1 + 1; // camera buffer + cube texture
        static constexpr uint32_t kGizmoDescriptorSetInfoCount = 1; // camera buffer
        static constexpr uint32_t kPickerDescriptorSetInfoCount = 1 + 1; // id image + out buffer
        static constexpr uint32_t kOutlineDescriptorSetInfoCount = 1; // id image
        static constexpr uint32_t kCullerDescriptorSetInfoCount = 4; // mesh data + template + out indirect + counter
        std::array<DescriptorSetInfo, kDescriptorSetInfoCount>        MakeMeshDescriptorSetInfo(const SkyboxPass &skybox, const TextureRegistry &textureRegistry) const;
        std::array<DescriptorSetInfo, kSkyboxDescriptorSetInfoCount>  MakeSkyboxDescriptorSetInfo(const Texture& skyboxTexture) const;
        std::array<DescriptorSetInfo, kGizmoDescriptorSetInfoCount>   MakeGizmoDescriptorSetInfo() const;
        std::array<DescriptorSetInfo, kPickerDescriptorSetInfoCount>  MakePickerDescriptorSetInfo() const;
        std::array<DescriptorSetInfo, kOutlineDescriptorSetInfoCount> MakeOutlineDescriptorSetInfo() const;
        std::array<DescriptorSetInfo, kCullerDescriptorSetInfoCount>  MakeCullerDescriptorSetInfo() const;
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

        Buffer                  mCameraBuffer;
        Buffer                  mMeshDataBuffer;
        Buffer                  mMaterialsBuffer;
        Buffer                  mIndirectBuffer;
        Buffer                  mSceneBuffer;
        Buffer                  mPickerBuffer;
        Buffer                  mCullerInputCommandsBuffer;
        Buffer                  mCullerCountBuffer;

        vk::Extent2D            mExtent;

        Texture                 mSceneTexture;
        Texture                 mOutIdTexture;
        Texture                 mResolvedOutIdTexture;
    };
}
