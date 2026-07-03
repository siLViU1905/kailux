#pragma once

#include "Core.h"
#include "Context.h"
#include "buffer/Buffer.h"
#include "command/OneTimeCommand.h"

namespace kailux
{
    struct BufferTransferResource
    {
        vk::Buffer              buffer;
        vk::DeviceSize          offset{};
        vk::DeviceSize          size{};
        vk::PipelineStageFlags2 dstStage{};
        vk::AccessFlags2        dstAccess{};
    };

    struct ImageUpload
    {
        vk::Image      image;
        vk::Buffer     staging;
        uint32_t       width{};
        uint32_t       height{};
        uint32_t       mipLevels{};
    };

    class TransferManager
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(TransferManager)

        static TransferManager create();

        struct RecordResult
        {
            std::vector<BufferTransferResource> resources;
            std::vector<Buffer>                 staging;
        };
        using OnRecord = std::move_only_function<RecordResult(vk::CommandBuffer)>;

        using OnComplete = std::move_only_function<void()>;

        void enqueueBuffer(
            const Context& context,
            OnRecord &&record,
            OnComplete &&onComplete
        );
        void enqueueImages(
            const Context &context,
            std::vector<ImageUpload> &&images,
            std::vector<Buffer> &&stagingOwnership,
            OnComplete &&onComplete
        );

        void poll(const Context& context);

        void drain(const Context& context);

        bool hasPending() const;

        void clear();

    private:
        struct PendingTransfer
        {
            OneTimeCommand                  transferCmd;
            OneTimeCommand                  graphicsCmd;
            vk::raii::Semaphore             semaphore;
            std::vector<Buffer>             staging;
            std::move_only_function<void()> onComplete;
        };
        void submitTransfer(
            const Context &context,
            OneTimeCommand &&transferCmd,
            OneTimeCommand &&graphicsCmd,
            std::vector<Buffer> &&staging,
            OnComplete &&onComplete
            );

        std::vector<PendingTransfer> mPending;
    };
}
