#include "TransferManager.h"

namespace kailux
{
    TransferManager::TransferManager() = default;

    TransferManager::TransferManager(TransferManager &&other) noexcept
        : m_Pending(std::move(other.m_Pending))
    {
    }

    TransferManager &TransferManager::operator=(TransferManager &&other) noexcept
    {
        if (this != &other)
        {
            m_Pending = std::move(other.m_Pending);
        }
        return *this;
    }

    TransferManager TransferManager::create()
    {
        return {};
    }

    void TransferManager::enqueue(const Context &context, OnRecord record, OnComplete onComplete)
    {
        auto graphicsFamily = context.getGraphicsQueueFamilyIndex();
        auto transferFamily = context.getTransferQueueFamilyIndex();
        bool needsOwnershipTransfer = context.hasDedicatedTransferQueue();

        auto transferOtc = OneTimeCommand::create(context, QueueType::Transfer);
        auto tCmd = transferOtc.getCommandBuffer();

        auto recorded = record(tCmd);

        const auto& resources = recorded.resources;
        if (needsOwnershipTransfer && !resources.empty())
        {
            std::vector<vk::BufferMemoryBarrier2> releaseBarriers;
            releaseBarriers.reserve(resources.size());
            for (const auto& res : resources)
                releaseBarriers.emplace_back(
                    vk::PipelineStageFlagBits2::eTransfer,
                    vk::AccessFlagBits2::eTransferWrite,
                    vk::PipelineStageFlagBits2::eNone,
                    vk::AccessFlagBits2::eNone,
                    transferFamily,
                    graphicsFamily,
                    res.buffer,
                    res.offset,
                    res.size
                    );

            vk::DependencyInfo depInfo{};
            depInfo.setBufferMemoryBarriers(releaseBarriers);
            tCmd.pipelineBarrier2(depInfo);
        }

        vk::raii::Semaphore semaphore(context.m_Device, vk::SemaphoreCreateInfo{});
        {
            tCmd.end();
            vk::SemaphoreSubmitInfo signalInfo(*semaphore, 0, vk::PipelineStageFlagBits2::eTransfer);
            vk::CommandBufferSubmitInfo cmdInfo(tCmd);
            vk::SubmitInfo2 submit({}, {}, cmdInfo, signalInfo);
            context.getTransferQueue().submit2(submit, nullptr);
        }

        auto acquireOtc = OneTimeCommand::create(context, QueueType::Graphics);
        auto aCmd = acquireOtc.getCommandBuffer();

        if (needsOwnershipTransfer && !resources.empty())
        {
            std::vector<vk::BufferMemoryBarrier2> acquireBarriers;
            acquireBarriers.reserve(resources.size());
            for (const auto &res : resources)
                acquireBarriers.emplace_back(
                    vk::PipelineStageFlagBits2::eNone,
                    vk::AccessFlagBits2::eNone,
                    res.dstStage,
                    res.dstAccess,
                    transferFamily,
                    graphicsFamily,
                    res.buffer,
                    res.offset,
                    res.size
                );

            vk::DependencyInfo depInfo{};
            depInfo.setBufferMemoryBarriers(acquireBarriers);
            aCmd.pipelineBarrier2(depInfo);
        }

        {
            aCmd.end();
            vk::SemaphoreSubmitInfo waitInfo(*semaphore, 0, vk::PipelineStageFlagBits2::eAllCommands);
            vk::CommandBufferSubmitInfo cmdInfo(aCmd);
            vk::SubmitInfo2 submit({}, waitInfo, cmdInfo, {});
            context.getGraphicsQueue().submit2(submit, acquireOtc.getFence());
        }

        m_Pending.emplace_back(
           std::move(transferOtc),
           std::move(acquireOtc),
           std::move(semaphore),
           std::move(recorded.staging),
           std::move(onComplete)
       );
    }

    void TransferManager::poll(const Context &context)
    {
        std::erase_if(m_Pending, [&](auto& pending)
        {
            auto status = context.getDevice().getFenceStatus(pending.acquireCmd.getFence());
            if (status == vk::Result::eSuccess)
            {
                pending.onComplete();
                return true;
            }
            return false;
        });
    }

    void TransferManager::drain(const Context &context)
    {
        for (auto &pending : m_Pending)
        {
            auto fence = pending.acquireCmd.getFence();
            auto result = context.getDevice().waitForFences(fence, true, UINT64_MAX);
            if (result != vk::Result::eSuccess)
                throw std::runtime_error("TransferManager::drain waitForFences failed");

            pending.onComplete();
        }
        m_Pending.clear();
    }

    bool TransferManager::hasPending() const
    {
        return !m_Pending.empty();
    }

    void TransferManager::clear()
    {
        m_Pending.clear();
    }
}
