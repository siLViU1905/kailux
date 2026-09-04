#pragma once
#include  <vulkan/vulkan_raii.hpp>

#include "core/buffer/Buffer.h"

namespace kailux
{
    struct ImageBarrier
    {
        vk::Image image;
        vk::ImageLayout oldLayout;
        vk::ImageLayout newLayout;
        vk::PipelineStageFlags2 srcStage = vk::PipelineStageFlagBits2::eAllCommands;
        vk::PipelineStageFlags2 dstStage = vk::PipelineStageFlagBits2::eAllCommands;
        vk::AccessFlags2 srcAccess = vk::AccessFlagBits2::eMemoryWrite;
        vk::AccessFlags2 dstAccess = vk::AccessFlagBits2::eMemoryRead |
                                     vk::AccessFlagBits2::eMemoryWrite;
        vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;
    };

    struct ColorAttachmentInfo
    {
        vk::ImageView view;
        vk::ImageView resolveView;
        vk::ImageLayout layout = vk::ImageLayout::eColorAttachmentOptimal;
        vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear;
        vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
        vk::ClearValue clearColor{std::array{0.f, 0.f, 0.f, 1.f}};
        vk::ResolveModeFlagBits resolveMode = vk::ResolveModeFlagBits::eNone;
    };

    struct RenderingInfo
    {
        std::span<const ColorAttachmentInfo> colorAttachments;
        vk::Extent2D                         extent;

        vk::ImageView         depthView{};
        vk::ImageLayout       depthLayout{vk::ImageLayout::eDepthAttachmentOptimal};
        vk::AttachmentLoadOp  depthLoadOp{vk::AttachmentLoadOp::eClear};
        vk::RenderingFlagBits renderFlags{};
    };

    class CommandRecorder
    {
    public:
        CommandRecorder(vk::CommandBuffer cmd);
        CommandRecorder(vk::CommandBuffer cmd, const vk::CommandBufferInheritanceRenderingInfo& inheritance);
        ~CommandRecorder();

        CommandRecorder(const CommandRecorder&) = delete;
        CommandRecorder& operator=(const CommandRecorder&) = delete;
        CommandRecorder(CommandRecorder&&) = delete;
        CommandRecorder& operator=(CommandRecorder&&) = delete;

        void ApplyImageBarrier(const ImageBarrier& info) const;
        void BufferMemoryBarriers(std::span<const vk::BufferMemoryBarrier2> barriers) const;

        void BeginRendering(const RenderingInfo& info);
        void EndRendering();
        void DrawIndexedIndirectCount(const Buffer& indirectBuffer, const Buffer &countBuffer, uint32_t maxDrawCount) const;

        void SetViewport(vk::Extent2D extent);
        void SetScissor(vk::Extent2D extent);

        vk::CommandBuffer GetCommandBuffer() const;

    private:
        vk::CommandBuffer mCmd;
        bool              mInRendering;
        bool              mIsSecondary;
    };
}
