#include "OneTimeCommand.h"

namespace kailux
{
    OneTimeCommand::OneTimeCommand() = default;

    OneTimeCommand::OneTimeCommand(OneTimeCommand &&other) noexcept : mQueueType(other.mQueueType),
                                                                      mCommandBuffer(std::move(other.mCommandBuffer)),
                                                                      mFence(std::move(other.mFence))
    {
    }

    OneTimeCommand &OneTimeCommand::operator=(OneTimeCommand &&other) noexcept
    {
        if (this != &other)
        {
            mQueueType = other.mQueueType;
            mCommandBuffer = std::move(other.mCommandBuffer);
            mFence = std::move(other.mFence);
        }
        return *this;
    }

    void OneTimeCommand::create_command_pools(const Context &context)
    {
        kGraphicsPool = vk::raii::CommandPool(
            context.mDevice,
            {
                vk::CommandPoolCreateFlagBits::eTransient,
                context.getGraphicsQueueFamilyIndex()
            }
        );

        kTransferPool = vk::raii::CommandPool(
            context.mDevice,
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
        otc.mQueueType = type;
        otc.createBuffer(context);
        otc.createFence(context);
        otc.mCommandBuffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        return otc;
    }

    void OneTimeCommand::submit(const Context &context) const
    {
        mCommandBuffer.end();

        const auto queue = (mQueueType == QueueType::Transfer)
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
        mCommandBuffer.end();

        auto queue = (mQueueType == QueueType::Transfer)
                        ? context.getTransferQueue()
                        : context.getGraphicsQueue();

        vk::CommandBufferSubmitInfo cmdInfo(getCommandBuffer());
        vk::SubmitInfo2 submitInfo({}, {}, cmdInfo, {});

        queue.submit2(submitInfo, *mFence);
    }

    vk::CommandBuffer OneTimeCommand::getCommandBuffer() const
    {
        return *mCommandBuffer;
    }

    vk::Fence OneTimeCommand::getFence() const
    {
        return *mFence;
    }

    void OneTimeCommand::createBuffer(const Context &context)
    {
        const auto &pool = (mQueueType == QueueType::Transfer) ? kTransferPool : kGraphicsPool;
        mCommandBuffer = std::move(vk::raii::CommandBuffers(
            context.mDevice,
            {
                *pool,
                vk::CommandBufferLevel::ePrimary,
                1
            }
        ).front());
    }

    void OneTimeCommand::createFence(const Context &context)
    {
        mFence = vk::raii::Fence(context.mDevice, vk::FenceCreateInfo{});
    }
}
