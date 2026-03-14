#include "FrameData.h"

namespace kailux
{
    FrameData::FrameData() : m_CommandPool({}),
                             m_CommandBuffer({}),
                             m_ImageAvailableSemaphore({}),
                             m_RenderFinishedSemaphore({}),
                             m_FenceInFlight({})
    {
    }

    FrameData::FrameData(FrameData &&other) noexcept : m_CommandPool(std::move(other.m_CommandPool)),
                                                       m_CommandBuffer(std::move(other.m_CommandBuffer)),
                                                       m_ImageAvailableSemaphore(std::move(other.m_ImageAvailableSemaphore)),
                                                       m_RenderFinishedSemaphore(std::move(other.m_RenderFinishedSemaphore)),
                                                       m_FenceInFlight(std::move(other.m_FenceInFlight))
    {
    }

    FrameData &FrameData::operator=(FrameData &&other) noexcept
    {
        if (this != &other)
        {
            m_CommandPool = std::move(other.m_CommandPool);
            m_CommandBuffer = std::move(other.m_CommandBuffer);
            m_ImageAvailableSemaphore = std::move(other.m_ImageAvailableSemaphore);
            m_RenderFinishedSemaphore = std::move(other.m_RenderFinishedSemaphore);
            m_FenceInFlight = std::move(other.m_FenceInFlight);
        }
        return *this;
    }

    FrameData FrameData::create(const Context &context)
    {
        FrameData frame;
        frame.createCommandPool(context);
        frame.createCommandBuffer(context);
        frame.createSyncObjects(context);
        return frame;
    }

    void FrameData::reset(const Context &context) const
    {
        auto result = context.getDevice().waitForFences(*m_FenceInFlight, true, UINT64_MAX);
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("waitForFences failed");

        context.getDevice().resetFences(*m_FenceInFlight);

        m_CommandPool.reset();
    }

    vk::CommandBuffer FrameData::getCommandBuffer() const
    {
        return *m_CommandBuffer;
    }

    vk::Semaphore FrameData::getImageAvailableSemaphore() const
    {
        return *m_ImageAvailableSemaphore;
    }

    vk::Semaphore FrameData::getRenderFinishedSemaphore() const
    {
        return *m_RenderFinishedSemaphore;
    }

    vk::Fence FrameData::getFenceInFlight() const
    {
        return *m_FenceInFlight;
    }

    void FrameData::createCommandPool(const Context &context)
    {
        vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        m_CommandPool = vk::raii::CommandPool(context.m_Device, poolInfo);
    }

    void FrameData::createCommandBuffer(const Context &context)
    {
        vk::CommandBufferAllocateInfo allocInfo(
           m_CommandPool,
           vk::CommandBufferLevel::ePrimary,
           1
       );

        m_CommandBuffer = std::move(vk::raii::CommandBuffers(context.m_Device, allocInfo).front());
    }

    void FrameData::createSyncObjects(const Context &context)
    {
        m_ImageAvailableSemaphore = vk::raii::Semaphore(context.m_Device, vk::SemaphoreCreateInfo());

        m_RenderFinishedSemaphore = vk::raii::Semaphore(context.m_Device, vk::SemaphoreCreateInfo());

        m_FenceInFlight = vk::raii::Fence(context.m_Device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }
}
