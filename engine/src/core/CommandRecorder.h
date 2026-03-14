#pragma once
#include  <vulkan/vulkan_raii.hpp>

namespace kailux
{

    struct ImageBarrier
    {
        vk::Image            image;
        vk::ImageLayout      oldLayout;
        vk::ImageLayout      newLayout;
        vk::PipelineStageFlags2 srcStage  = vk::PipelineStageFlagBits2::eAllCommands;
        vk::PipelineStageFlags2 dstStage  = vk::PipelineStageFlagBits2::eAllCommands;
        vk::AccessFlags2     srcAccess   = vk::AccessFlagBits2::eMemoryWrite;
        vk::AccessFlags2     dstAccess   = vk::AccessFlagBits2::eMemoryRead |
                                           vk::AccessFlagBits2::eMemoryWrite;
    };

    struct RenderingInfo
    {
        vk::ImageView   colorView;
        vk::Extent2D    extent;
        vk::ImageLayout colorLayout  = vk::ImageLayout::eColorAttachmentOptimal;
        vk::AttachmentLoadOp  loadOp  = vk::AttachmentLoadOp::eClear;
        vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
        vk::ClearColorValue   clearColor { std::array{ 0.f, 0.f, 0.f, 1.f } };

        vk::ImageView   depthView   {};
        vk::ImageLayout depthLayout = vk::ImageLayout::eDepthAttachmentOptimal;
    };

    class CommandRecorder
    {
    public:
        CommandRecorder(vk::CommandBuffer cmd);
        ~CommandRecorder();

        CommandRecorder(const CommandRecorder&) = delete;
        CommandRecorder& operator=(const CommandRecorder&) = delete;
        CommandRecorder(CommandRecorder&&) = delete;
        CommandRecorder& operator=(CommandRecorder&&) = delete;

        void barrier(const ImageBarrier& info);

        void beginRendering(const RenderingInfo& info);
        void endRendering();

        void setViewport(vk::Extent2D extent);
        void setScissor(vk::Extent2D extent);

        vk::CommandBuffer getCommandBuffer() const;

    private:
        vk::CommandBuffer m_Cmd;
        bool              m_InRendering;
    };
}