#include "TransferManager.h"

#include "texture/TextureAllocator.h"

namespace kailux
{
    TransferManager::TransferManager() = default;

    TransferManager::TransferManager(TransferManager &&other) noexcept
        : mPending(std::move(other.mPending))
    {
    }

    TransferManager &TransferManager::operator=(TransferManager &&other) noexcept
    {
        if (this != &other)
        {
            mPending = std::move(other.mPending);
        }
        return *this;
    }

    TransferManager TransferManager::create()
    {
        return {};
    }

    void TransferManager::EnqueueBuffer(const Context &context, OnRecord &&record, OnComplete &&onComplete)
    {
        auto graphicsFamily = context.GetGraphicsQueueFamilyIndex();
        auto transferFamily = context.GetTransferQueueFamilyIndex();
        bool needsOwnershipTransfer = context.HasDedicatedTransferQueue();

        auto transferOtc = OneTimeCommand::create(context, QueueType::Transfer);
        auto tCmd = transferOtc.GetCommandBuffer();

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

        auto graphicsOtc = OneTimeCommand::create(context, QueueType::Graphics);
        auto gCmd = graphicsOtc.GetCommandBuffer();

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
            gCmd.pipelineBarrier2(depInfo);
        }

        SubmitTransfer(
            context,
            std::move(transferOtc),
            std::move(graphicsOtc),
            std::move(recorded.staging),
            std::move(onComplete)
        );
    }

    void TransferManager::EnqueueImages(
        const Context &context,
        std::vector<ImageUpload> &&images,
        std::vector<Buffer> &&stagingOwnership,
        OnComplete &&onComplete
    )
    {
        if (images.empty())
            return;

        auto transferFamily = context.GetTransferQueueFamilyIndex();
        auto graphicsFamily = context.GetGraphicsQueueFamilyIndex();
        bool needsOwnershipTransfer = context.HasDedicatedTransferQueue();

        auto transferOtc = OneTimeCommand::create(context, QueueType::Transfer);
        auto tCmd = transferOtc.GetCommandBuffer();

        for (const auto &img : images)
            TextureAllocator::record_texture_copy(
                tCmd, img.image, img.staging, img.width, img.height, img.mipLevels);

        if (needsOwnershipTransfer)
        {
            std::vector<vk::ImageMemoryBarrier2> releaseBarriers;
            releaseBarriers.reserve(images.size());
            for (const auto &img: images)
                releaseBarriers.emplace_back(
                    vk::PipelineStageFlagBits2::eTransfer,
                    vk::AccessFlagBits2::eTransferWrite,
                    vk::PipelineStageFlagBits2::eNone,
                    vk::AccessFlagBits2::eNone,
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::eTransferDstOptimal,
                    transferFamily,
                    graphicsFamily,
                    img.image,
                    vk::ImageSubresourceRange{
                        vk::ImageAspectFlagBits::eColor,
                        0,
                        img.mipLevels,
                    0,
                    1
                }
            );
            vk::DependencyInfo depInfo{};
            depInfo.setImageMemoryBarriers(releaseBarriers);
            tCmd.pipelineBarrier2(depInfo);
        }

        auto graphicsOtc = OneTimeCommand::create(context, QueueType::Graphics);
        auto gCmd = graphicsOtc.GetCommandBuffer();

        if (needsOwnershipTransfer)
        {
            std::vector<vk::ImageMemoryBarrier2> acquireBarriers;
            acquireBarriers.reserve(images.size());
            for (const auto &img: images)
                acquireBarriers.emplace_back(
                    vk::PipelineStageFlagBits2::eNone,
                    vk::AccessFlagBits2::eNone,
                    vk::PipelineStageFlagBits2::eTransfer,
                    vk::AccessFlagBits2::eTransferRead | vk::AccessFlagBits2::eTransferWrite,
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::eTransferDstOptimal,
                    transferFamily,
                    graphicsFamily,
                    img.image,
                    vk::ImageSubresourceRange{
                        vk::ImageAspectFlagBits::eColor,
                        0,
                        img.mipLevels,
                        0,
                        1
                    }
                );
            vk::DependencyInfo depInfo{};
            depInfo.setImageMemoryBarriers(acquireBarriers);
            gCmd.pipelineBarrier2(depInfo);
        }

        for (const auto &img : images)
            TextureAllocator::record_texture_mipmaps(gCmd, img.image, img.width, img.height, img.mipLevels);

        SubmitTransfer(
            context,
            std::move(transferOtc),
            std::move(graphicsOtc),
            std::move(stagingOwnership),
            std::move(onComplete)
        );
    }

    void TransferManager::Poll(const Context &context)
    {
        std::erase_if(mPending, [&](auto& pending)
        {
            auto status = context.GetDevice().getFenceStatus(pending.graphicsCmd.GetFence());
            if (status == vk::Result::eSuccess)
            {
                pending.onComplete();
                return true;
            }
            return false;
        });
    }

    void TransferManager::Drain(const Context &context)
    {
        for (auto &pending : mPending)
        {
            auto fence = pending.graphicsCmd.GetFence();
            auto result = context.GetDevice().waitForFences(fence, true, UINT64_MAX);
            if (result != vk::Result::eSuccess)
                throw std::runtime_error("TransferManager::drain waitForFences failed");

            pending.onComplete();
        }
        mPending.clear();
    }

    bool TransferManager::HasPending() const
    {
        return !mPending.empty();
    }

    void TransferManager::Clear()
    {
        mPending.clear();
    }

    void TransferManager::SubmitTransfer(
        const Context &context,
        OneTimeCommand &&transferCmd,
        OneTimeCommand &&graphicsCmd,
        std::vector<Buffer> &&staging,
        OnComplete &&onComplete
    )
    {
        vk::raii::Semaphore semaphore(context.mDevice, vk::SemaphoreCreateInfo{});

        auto tCmd = transferCmd.GetCommandBuffer();
        auto gCmd = graphicsCmd.GetCommandBuffer();

        {
            tCmd.end();
            vk::SemaphoreSubmitInfo signalInfo(*semaphore, 0, vk::PipelineStageFlagBits2::eTransfer);
            vk::CommandBufferSubmitInfo cmdInfo(tCmd);
            vk::SubmitInfo2 submit({}, {}, cmdInfo, signalInfo);
            context.GetTransferQueue().submit2(submit, nullptr);
        }

        {
            gCmd.end();
            vk::SemaphoreSubmitInfo waitInfo(*semaphore, 0, vk::PipelineStageFlagBits2::eAllCommands);
            vk::CommandBufferSubmitInfo cmdInfo(gCmd);
            vk::SubmitInfo2 submit({}, waitInfo, cmdInfo, {});
            context.GetGraphicsQueue().submit2(submit, graphicsCmd.GetFence());
        }

        mPending.emplace_back(
            std::move(transferCmd),
            std::move(graphicsCmd),
            std::move(semaphore),
            std::move(staging),
            std::move(onComplete)
        );
    }
}
