#include "OneTimeCommand.h"

namespace kailux
{
    OneTimeCommand::OneTimeCommand() = default;

    OneTimeCommand::OneTimeCommand(OneTimeCommand &&other) noexcept : m_QueueType(other.m_QueueType),
                                                                      m_CommandBuffer(std::move(other.m_CommandBuffer)),
                                                                      m_Fence(std::move(other.m_Fence))
    {
    }

    OneTimeCommand &OneTimeCommand::operator=(OneTimeCommand &&other) noexcept
    {
        if (this != &other)
        {
            m_QueueType = other.m_QueueType;
            m_CommandBuffer = std::move(other.m_CommandBuffer);
            m_Fence = std::move(other.m_Fence);
        }
        return *this;
    }

    void OneTimeCommand::create_command_pools(const Context &context)
    {
        kGraphicsPool = vk::raii::CommandPool(
            context.m_Device,
            {
                vk::CommandPoolCreateFlagBits::eTransient,
                context.getGraphicsQueueFamilyIndex()
            }
        );

        kTransferPool = vk::raii::CommandPool(
            context.m_Device,
            {
                vk::CommandPoolCreateFlagBits::eTransient,
                context.getTransferQueueFamilyIndex()
            }
        );
    }

    void OneTimeCommand::destroy_command_pools()
    {
        kGraphicsPool = {nullptr};
        kTransferPool = {nullptr};
    }

    OneTimeCommand OneTimeCommand::create(const Context &context, QueueType type)
    {
        OneTimeCommand otc;
        otc.m_QueueType = type;
        otc.createBuffer(context);
        otc.createFence(context);
        otc.m_CommandBuffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        return otc;
    }

    void OneTimeCommand::submit(const Context &context) const
    {
        m_CommandBuffer.end();

        const auto queue = (m_QueueType == QueueType::Transfer)
                        ? context.getTransferQueue()
                        : context.getGraphicsQueue();

        vk::CommandBufferSubmitInfo cmdInfo(getCommandBuffer());
        vk::SubmitInfo2 submitInfo(
            {},
            {},
            cmdInfo,
            {}
        );
        queue.submit2(submitInfo, nullptr);
        queue.waitIdle();
    }

    void OneTimeCommand::submitAsync(const Context &context) const
    {
        m_CommandBuffer.end();

        auto queue = (m_QueueType == QueueType::Transfer)
                        ? context.getTransferQueue()
                        : context.getGraphicsQueue();

        vk::CommandBufferSubmitInfo cmdInfo(getCommandBuffer());
        vk::SubmitInfo2 submitInfo({}, {}, cmdInfo, {});

        queue.submit2(submitInfo, *m_Fence);
    }

    vk::CommandBuffer OneTimeCommand::getCommandBuffer() const
    {
        return *m_CommandBuffer;
    }

    vk::Fence OneTimeCommand::getFence() const
    {
        return *m_Fence;
    }

    void OneTimeCommand::createBuffer(const Context &context)
    {
        const auto &pool = (m_QueueType == QueueType::Transfer) ? kTransferPool : kGraphicsPool;
        m_CommandBuffer = std::move(vk::raii::CommandBuffers(
            context.m_Device,
            {
                *pool,
                vk::CommandBufferLevel::ePrimary,
                1
            }
        ).front());
    }

    void OneTimeCommand::createFence(const Context &context)
    {
        m_Fence = vk::raii::Fence(context.m_Device, vk::FenceCreateInfo{});
    }
}
