#pragma once
#include  <vulkan/vulkan_raii.hpp>

#include "core/buffer/Buffer.h"

namespace kailux
{

    struct ImageBarrier
    {
        vk::Image               image;
        vk::ImageLayout         oldLayout;
        vk::ImageLayout         newLayout;
        vk::PipelineStageFlags2 srcStage    = vk::PipelineStageFlagBits2::eAllCommands;
        vk::PipelineStageFlags2 dstStage    = vk::PipelineStageFlagBits2::eAllCommands;
        vk::AccessFlags2        srcAccess   = vk::AccessFlagBits2::eMemoryWrite;
        vk::AccessFlags2        dstAccess   = vk::AccessFlagBits2::eMemoryRead |
                                              vk::AccessFlagBits2::eMemoryWrite;
        vk::ImageAspectFlags    aspect      = vk::ImageAspectFlagBits::eColor;
    };

    struct RenderingInfo
    {
        vk::ImageView          colorView;
        vk::ImageView          resolveView;
        vk::Extent2D           extent;
        vk::ImageLayout        colorLayout  = vk::ImageLayout::eColorAttachmentOptimal;
        vk::AttachmentLoadOp   loadOp  = vk::AttachmentLoadOp::eClear;
        vk::AttachmentStoreOp  storeOp = vk::AttachmentStoreOp::eStore;
        vk::ClearColorValue    clearColor { std::array{ 0.f, 0.f, 0.f, 1.f } };

        vk::ImageView          depthView{};
        vk::ImageLayout        depthLayout = vk::ImageLayout::eDepthAttachmentOptimal;
        vk::RenderingFlagBits  renderFlags{};
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

        void imageBarrier(const ImageBarrier& info) const;
        void bufferMemoryBarriers(std::span<const vk::BufferMemoryBarrier2> barriers) const;

        void beginRendering(const RenderingInfo& info);
        void endRendering();
        void drawIndexedIndirect(const Buffer& indirectBuffer, uint32_t drawCount) const;

        void setViewport(vk::Extent2D extent);
        void setScissor(vk::Extent2D extent);

        vk::CommandBuffer getCommandBuffer() const;

    private:
        vk::CommandBuffer m_Cmd;
        bool              m_InRendering;
        bool              m_IsSecondary;
    };
}
