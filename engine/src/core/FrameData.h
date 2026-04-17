#pragma once
#include "Context.h"
#include "SkyboxPass.h"
#include "buffer/Buffer.h"
#include "command/CommandRecorder.h"
#include "descriptor/DescriptorSet.h"
#include "texture/Texture.h"
#include "texture/TextureRegistry.h"

namespace kailux
{
    class FrameData
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(FrameData)

        static FrameData create(const Context &context,
                                const DescriptorLayout &descriptorLayout,
                                const DescriptorPool &descriptorPool,
                                const SkyboxPass &skybox,
                                const TextureRegistry &textureRegistry,
                                uint32_t maxMeshCount
        );

        void reset(const Context& context) const;

        vk::CommandBuffer getCommandBuffer() const;
        vk::CommandBuffer getImGuiCommandBuffer() const;
        vk::Fence         getFenceInFlight() const;

        const DescriptorSet& getDescriptorSet() const;
        const DescriptorSet& getSkyboxDescriptorSet() const;

        Buffer&       getCameraBuffer();
        Buffer&       getModelBuffer();
        Buffer&       getIndirectBuffer();
        const Buffer& getIndirectBuffer() const;
        Buffer&       getSceneBuffer();

        static constexpr uint32_t s_BufferMemoryBarriersCount = 1 + 1 + 1 + 1; // camera buffer + mesh data buffer + indirect buffer + scene buffer
        std::array<vk::BufferMemoryBarrier2, s_BufferMemoryBarriersCount> getBufferMemoryBarriers() const;

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
        void createCameraBuffer(const Context& context);
        void createMeshDataBuffer(const Context& context, uint32_t meshCount);
        void createIndirectBuffer(const Context& context, uint32_t count);
        void createSceneBuffer(const Context& context);

        static constexpr uint32_t s_DescriptorSetInfoCount = 1 + 1 + 1 + 1 + 1 + 1 + TextureRegistry::s_TextureTypes.size(); // camera buffer + mesh data buffer + scene buffer + skybox sampler + irradiance map + brdf lut + textures
        static constexpr uint32_t s_SkyboxDescriptorSetInfoCount = 1 + 1; // camera buffer + cube texture
        std::array<DescriptorSetInfo, s_DescriptorSetInfoCount> makeDescriptorSetInfo(const SkyboxPass &skybox, const TextureRegistry& textureRegistry, uint32_t meshCount) const;
        std::array<DescriptorSetInfo, s_SkyboxDescriptorSetInfoCount> makeSkyboxDescriptorSetInfo(const Texture& skyboxTexture) const;

        vk::raii::CommandPool   m_CommandPool;
        vk::raii::CommandPool   m_ImGuiCommandPool;
        vk::raii::CommandBuffer m_CommandBuffer;
        vk::raii::CommandBuffer m_ImGuiCommandBuffer;
        vk::raii::Fence         m_FenceInFlight;

        DescriptorSet           m_DescriptorSet;
        DescriptorSet           m_SkyboxDescriptorSet;
        Buffer                  m_CameraBuffer;
        Buffer                  m_MeshDataBuffer;
        Buffer                  m_IndirectBuffer;
        Buffer                  m_SceneBuffer;
    };
}
