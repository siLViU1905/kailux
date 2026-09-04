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
        void Submit(const Context& context) const;
        void SubmitAsync(const Context& context) const;

        vk::CommandBuffer GetCommandBuffer() const;
        vk::Fence         GetFence() const;

    private:
        void CreateBuffer(const Context& context);
        void CreateFence(const Context& context);

        inline static vk::raii::CommandPool   kGraphicsPool{nullptr};
        inline static vk::raii::CommandPool   kTransferPool{nullptr};

        QueueType                      mQueueType{QueueType::Graphics};
        vk::raii::CommandBuffer        mCommandBuffer{nullptr};
        vk::raii::Fence                mFence{nullptr};
    };
}
