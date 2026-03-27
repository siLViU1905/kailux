#include "OneTimeCommand.h"

namespace kailux
{
    OneTimeCommand::OneTimeCommand() : m_CommandBuffer({})
    {
    }

    OneTimeCommand::OneTimeCommand(OneTimeCommand &&other) noexcept : m_CommandBuffer(std::move(other.m_CommandBuffer))
    {
    }

    OneTimeCommand &OneTimeCommand::operator=(OneTimeCommand &&other) noexcept
    {
        if (this != &other)
        {
            m_CommandBuffer = std::move(other.m_CommandBuffer);
        }
        return *this;
    }

    vk::raii::CommandPool OneTimeCommand::s_CommandPool{nullptr};

    void OneTimeCommand::create_command_pool(const Context &context)
    {
        s_CommandPool = vk::raii::CommandPool(
            context.m_Device,
            {
                vk::CommandPoolCreateFlagBits::eTransient,
                context.getGraphicsQueueFamilyIndex()
            }
        );
    }

    void OneTimeCommand::destroy_command_pool()
    {
        s_CommandPool = {nullptr};
    }

    OneTimeCommand OneTimeCommand::create(const Context &context)
    {
        OneTimeCommand otc;
        otc.createBuffer(context);
        otc.m_CommandBuffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        return otc;
    }

    void OneTimeCommand::submit(const Context &context) const
    {
        m_CommandBuffer.end();

        vk::CommandBufferSubmitInfo cmdInfo(getCommandBuffer());
        vk::SubmitInfo2 submitInfo(
            {},
            {},
            cmdInfo,
            {}
        );
        context.getGraphicsQueue().submit2(submitInfo, nullptr);
        context.getGraphicsQueue().waitIdle();
    }

    vk::CommandBuffer OneTimeCommand::getCommandBuffer() const
    {
        return *m_CommandBuffer;
    }

    void OneTimeCommand::createBuffer(const Context &context)
    {
        m_CommandBuffer = std::move(vk::raii::CommandBuffers(
            context.m_Device,
            {
                *s_CommandPool,
                vk::CommandBufferLevel::ePrimary,
                1
            }
        ).front());
    }
}
