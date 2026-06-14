#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "../Context.h"
#include "../Core.h"

namespace kailux
{
    enum class QueueType : uint8_t
    {
        Graphics,
        Transfer
    };

    class OneTimeCommand
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(OneTimeCommand)

        static void create_command_pools(const Context& context);
        static void destroy_command_pools();

        //already calls begin()
        static OneTimeCommand create(const Context& context, QueueType type = QueueType::Graphics);

        //already calls end()
        void submit(const Context& context) const;
        void submitAsync(const Context& context) const;

        vk::CommandBuffer getCommandBuffer() const;
        vk::Fence         getFence() const;

    private:
        void createBuffer(const Context& context);
        void createFence(const Context& context);

        inline static vk::raii::CommandPool   s_GraphicsPool{nullptr};
        inline static vk::raii::CommandPool   s_TransferPool{nullptr};

        QueueType                      m_QueueType{QueueType::Graphics};
        vk::raii::CommandBuffer        m_CommandBuffer{nullptr};
        vk::raii::Fence                m_Fence{nullptr};
    };
}
