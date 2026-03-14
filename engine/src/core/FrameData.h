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
        vk::Fence         getFenceInFlight() const;

    private:
        void createCommandPool(const Context& context);
        void createCommandBuffer(const Context& context);
        void createSyncObjects(const Context& context);

        vk::raii::CommandPool   m_CommandPool;
        vk::raii::CommandBuffer m_CommandBuffer;
        vk::raii::Fence         m_FenceInFlight;
    };
}
