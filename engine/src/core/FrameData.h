#pragma once
#include "Context.h"
#include "SkyboxPass.h"
#include "buffer/Buffer.h"
#include "command/CommandRecorder.h"
#include "descriptor/DescriptorSet.h"
#include "texture/Texture.h"
#include "texture/TextureRegistry.h"
#include "ComputePicker.h"

namespace kailux
{
    class FrameData
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(FrameData)

        static FrameData create(const Context &context,
                                const Swapchain &swapchain,
                                const DescriptorLayout &descriptorLayout,
                                const DescriptorPool &descriptorPool,
                                const SkyboxPass &skybox,
                                const ComputePicker& picker,
                                const TextureRegistry &textureRegistry,
                                uint32_t maxMeshCount
        );

        void reset(const Context& context) const;

        void recreateTextures(const Context& context, const Swapchain& swapchain);

        vk::CommandBuffer getCommandBuffer() const;
        vk::CommandBuffer getImGuiCommandBuffer() const;
        vk::Fence         getFenceInFlight() const;

        const DescriptorSet& getDescriptorSet() const;
        const DescriptorSet& getSkyboxDescriptorSet() const;
        const DescriptorSet& getPickerDescriptorSet() const;

        Buffer&       getCameraBuffer();
        Buffer&       getModelBuffer();
        Buffer&       getIndirectBuffer();
        const Buffer& getIndirectBuffer() const;
        Buffer&       getSceneBuffer();
        const Buffer& getPickerBuffer() const;

        vk::Extent2D  getExtent() const;

        const Texture& getSceneTexture() const;
        const Texture& getOutIdTexture() const;
        const Texture& getResolvedOutIdTexture() const;

        static constexpr uint32_t s_BufferMemoryBarriersCount = 1 + 1 + 1 + 1; // camera buffer + mesh data buffer + indirect buffer + scene buffer
        std::array<vk::BufferMemoryBarrier2, s_BufferMemoryBarriersCount> getBufferMemoryBarriers() const;
        vk::BufferMemoryBarrier2                                          getPickerBufferMemoryBarrier() const;

    private:
        void createCommandPool(const Context& context);
        //Separate command pool for future imgui separate thread integration
        void createImGuiCommandPool(const Context& context);
        void createCommandBuffer(const Context& context);
        void createImGuiCommandBuffer(const Context& context);
        void createSyncObjects(const Context& context);
        void createDescriptorSet(const Context& context, const DescriptorLayout& descriptorLayout, const DescriptorPool& descriptorPool, std::span<const
                                 DescriptorSetInfo> infos);
        void createSkyboxDescriptorSet(const Context& context, const DescriptorLayout& descriptorLayout, const DescriptorPool& descriptorPool, std::span<const
                                       DescriptorSetInfo> infos);
        void createPickerDescriptorSet(const Context& context, const DescriptorLayout& descriptorLayout, const DescriptorPool& descriptorPool, std::span<const
                                       DescriptorSetInfo> infos);
        void createCameraBuffer(const Context& context);
        void createMeshDataBuffer(const Context& context, uint32_t meshCount);
        void createIndirectBuffer(const Context& context, uint32_t count);
        void createSceneBuffer(const Context& context);
        void createPickerBuffer(const Context& context);

        void createSceneTexture(const Context& context, vk::Format format);
        void createOutIdTexture(const Context &context);

        static constexpr uint32_t s_DescriptorSetInfoCount = 1 + 1 + 1 + 1 + 1 + 1 + 1 + TextureRegistry::s_TextureTypes.size(); // camera buffer + mesh data buffer + scene buffer + skybox sampler + irradiance map + prefiltered env + brdf lut + textures
        static constexpr uint32_t s_SkyboxDescriptorSetInfoCount = 1 + 1; // camera buffer + cube texture
        static constexpr uint32_t s_PickerDescriptorSetInfoCount = 1 + 1; // id image + out buffer
        std::array<DescriptorSetInfo, s_DescriptorSetInfoCount>       makeDescriptorSetInfo(const SkyboxPass &skybox, const TextureRegistry& textureRegistry, uint32_t meshCount) const;
        std::array<DescriptorSetInfo, s_SkyboxDescriptorSetInfoCount> makeSkyboxDescriptorSetInfo(const Texture& skyboxTexture) const;
        std::array<DescriptorSetInfo, s_PickerDescriptorSetInfoCount> makePickerDescriptorSetInfo() const;
        static constexpr uint32_t s_PickerResolvedViewDescriptorSetBinding = 0;

        vk::raii::CommandPool   m_CommandPool;
        vk::raii::CommandPool   m_ImGuiCommandPool;
        vk::raii::CommandBuffer m_CommandBuffer;
        vk::raii::CommandBuffer m_ImGuiCommandBuffer;
        vk::raii::Fence         m_FenceInFlight;

        DescriptorSet           m_DescriptorSet;
        DescriptorSet           m_SkyboxDescriptorSet;
        DescriptorSet           m_PickerDescriptorSet;

        Buffer                  m_CameraBuffer;
        Buffer                  m_MeshDataBuffer;
        Buffer                  m_IndirectBuffer;
        Buffer                  m_SceneBuffer;
        Buffer                  m_PickerBuffer;

        vk::Extent2D            m_Extent;

        Texture                 m_SceneTexture;
        Texture                 m_OutIdTexture;
        Texture                 m_ResolvedOutIdTexture;
    };
}
