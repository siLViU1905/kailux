#pragma once

#include "Core.h"
#include "Context.h"
#include "buffer/Buffer.h"
#include "command/OneTimeCommand.h"

namespace kailux
{
    struct TransferResource
    {
        vk::Buffer              buffer;
        vk::DeviceSize          offset{};
        vk::DeviceSize          size{};
        vk::PipelineStageFlags2 dstStage{};
        vk::AccessFlags2        dstAccess{};
    };

    class TransferManager
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(TransferManager)

        static TransferManager create();

        struct RecordResult
        {
            std::vector<TransferResource> resources;
            std::vector<Buffer>           staging;
        };
        using OnRecord = std::move_only_function<RecordResult(vk::CommandBuffer)>;

        using OnComplete = std::move_only_function<void()>;

        void enqueue(
            const Context& context,
            OnRecord record,
            OnComplete onComplete
        );

        void poll(const Context& context);

        void drain(const Context& context);

        bool hasPending() const;

        void clear();

    private:
        struct PendingTransfer
        {
            OneTimeCommand                  transferCmd;
            OneTimeCommand                  acquireCmd;
            vk::raii::Semaphore             semaphore;
            std::vector<Buffer>             staging;
            std::move_only_function<void()> onComplete;
        };

        std::vector<PendingTransfer> m_Pending;
    };
}
