#pragma once
#include "Context.h"

namespace kailux
{
    class FrameData
    {
    public:
        FrameData();
        FrameData(const FrameData&) = delete;
        FrameData& operator=(const FrameData&) = delete;
        FrameData(FrameData&& other) noexcept;
        FrameData& operator=(FrameData&& other) noexcept;

        static FrameData create(const Context &context);

        void reset(const Context& context) const;

        vk::CommandBuffer getCommandBuffer() const;
        vk::CommandBuffer getImGuiCommandBuffer() const;
        vk::Fence         getFenceInFlight() const;

    private:
        void createCommandPool(const Context& context);
        //Separate command pool for future imgui separate thread integration
        void createImGuiCommandPool(const Context& context);
        void createCommandBuffer(const Context& context);
        void createImGuiCommandBuffer(const Context& context);
        void createSyncObjects(const Context& context);

        vk::raii::CommandPool   m_CommandPool;
        vk::raii::CommandPool   m_ImGuiCommandPool;
        vk::raii::CommandBuffer m_CommandBuffer;
        vk::raii::CommandBuffer m_ImGuiCommandBuffer;
        vk::raii::Fence         m_FenceInFlight;
    };
}
