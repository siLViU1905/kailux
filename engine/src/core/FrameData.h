#pragma once
#include "Context.h"
#include "buffer/Buffer.h"
#include "command/CommandRecorder.h"
#include "descriptor/DescriptorSet.h"

namespace kailux
{
    class FrameData
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(FrameData)

        static FrameData create(const Context &context,
                                const DescriptorLayout& descriptorLayout,
                                const DescriptorPool& descriptorPool,
                                uint32_t maxMeshCount);

        void reset(const Context& context) const;

        vk::CommandBuffer getCommandBuffer() const;
        vk::CommandBuffer getImGuiCommandBuffer() const;
        vk::Fence         getFenceInFlight() const;

        const DescriptorSet& getDescriptorSet() const;

        Buffer&       getCameraBuffer();
        Buffer&       getIndirectBuffer();
        const Buffer& getIndirectBuffer() const;

        static constexpr uint32_t s_BufferMemoryBarriersCount = 1 + 1; // camera buffer + indirect buffer
        std::array<vk::BufferMemoryBarrier2, s_BufferMemoryBarriersCount> getBufferMemoryBarriers() const;

    private:
        void createCommandPool(const Context& context);
        //Separate command pool for future imgui separate thread integration
        void createImGuiCommandPool(const Context& context);
        void createCommandBuffer(const Context& context);
        void createImGuiCommandBuffer(const Context& context);
        void createSyncObjects(const Context& context);
        void createDescriptorSet(const Context& context, const DescriptorLayout& descriptorLayout, const DescriptorPool& descriptorPool, std::span<DescriptorSetInfo> infos);
        void createCameraBuffer(const Context& context);
        void createIndirectBuffer(const Context& context, uint32_t count);

        std::array<DescriptorSetInfo, 1> makeDescriptorSetInfo() const;

        vk::raii::CommandPool   m_CommandPool;
        vk::raii::CommandPool   m_ImGuiCommandPool;
        vk::raii::CommandBuffer m_CommandBuffer;
        vk::raii::CommandBuffer m_ImGuiCommandBuffer;
        vk::raii::Fence         m_FenceInFlight;

        DescriptorSet             m_DescriptorSet;
        Buffer                    m_CameraBuffer;
        Buffer                    m_IndirectBuffer;
    };
}
