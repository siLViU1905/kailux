#include "CommandRecorder.h"

#include "../Log.h"

namespace kailux
{
    CommandRecorder::CommandRecorder(vk::CommandBuffer cmd) : mCmd(cmd), mInRendering(false), mIsSecondary(false)
    {
        vk::CommandBufferBeginInfo beginInfo{
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        };

        mCmd.begin(beginInfo);
    }

    CommandRecorder::CommandRecorder(vk::CommandBuffer cmd,
                                     const vk::CommandBufferInheritanceRenderingInfo &inheritance) : mCmd(cmd),
        mInRendering(false), mIsSecondary(true)
    {
        vk::CommandBufferInheritanceInfo inheritanceInfo;
        inheritanceInfo.pNext = &inheritance;

        vk::CommandBufferBeginInfo beginInfo{
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eRenderPassContinue,
            &inheritanceInfo
        };

        mCmd.begin(beginInfo);
    }

    CommandRecorder::~CommandRecorder()
    {
        if (mInRendering && !mIsSecondary)
            mCmd.endRendering();

        mCmd.end();
    }

    void CommandRecorder::imageBarrier(const ImageBarrier &info) const
    {
        vk::ImageMemoryBarrier2 imageBarrier{
            info.srcStage,
            info.srcAccess,
            info.dstStage,
            info.dstAccess,
            info.oldLayout,
            info.newLayout,
            vk::QueueFamilyIgnored,
            vk::QueueFamilyIgnored,
            info.image,
            vk::ImageSubresourceRange{
                info.aspect,
                0, 1,
                0, 1
            }
        };

        vk::DependencyInfo depInfo{};
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &imageBarrier;

        mCmd.pipelineBarrier2(depInfo);
    }

    void CommandRecorder::bufferMemoryBarriers(std::span<const vk::BufferMemoryBarrier2> barriers) const
    {
        vk::DependencyInfo depInfo{};
        depInfo.setBufferMemoryBarriers(barriers);

        mCmd.pipelineBarrier2(depInfo);
    }

    void CommandRecorder::beginRendering(const RenderingInfo &info)
    {
        std::vector<vk::RenderingAttachmentInfo> vkColorAttachments;
        vkColorAttachments.reserve(info.colorAttachments.size());

        for (const auto& color : info.colorAttachments)
        {
            vk::RenderingAttachmentInfo att(
                color.view,
                color.layout,
                vk::ResolveModeFlagBits::eNone,
                {},
                {},
                color.loadOp,
                color.storeOp,
                color.clearColor
            );

            if (color.resolveView)
            {
                att.resolveMode = color.resolveMode;
                att.resolveImageView = color.resolveView;
                att.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
            }
            vkColorAttachments.push_back(att);
        }

        vk::RenderingInfo renderingInfo(
            info.renderFlags,
            {{0, 0}, info.extent},
            1,
            0,
            static_cast<uint32_t>(vkColorAttachments.size()),
            vkColorAttachments.data(),
            nullptr,
            nullptr
        );

        vk::RenderingAttachmentInfo depthAttachment;
        if (info.depthView)
        {
            depthAttachment = {
                info.depthView,
                info.depthLayout,
                vk::ResolveModeFlagBits::eNone,
                {},
                {},
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::ClearValue{vk::ClearDepthStencilValue{1.0f, 0}}
            };
            renderingInfo.pDepthAttachment = &depthAttachment;
        }

        mCmd.beginRendering(renderingInfo);
        mInRendering = true;
    }

    void CommandRecorder::endRendering()
    {
        if (!mInRendering || mIsSecondary)
        {
            KAILUX_LOG_WARNING("[CommandRecorder]",
                               "endRendering() was called while the command was rendering or from a secondary buffer")
            return;
        }

        mCmd.endRendering();
        mInRendering = false;
    }

    void CommandRecorder::drawIndexedIndirectCount(const Buffer &indirectBuffer, const Buffer &countBuffer, uint32_t maxDrawCount) const
    {
        mCmd.drawIndexedIndirectCount(
            indirectBuffer.getBuffer(),
            {},
            countBuffer.getBuffer(),
            {},
            maxDrawCount,
            sizeof(vk::DrawIndexedIndirectCommand)
        );
    }

    void CommandRecorder::setViewport(vk::Extent2D extent)
    {
        vk::Viewport viewport{
            0.f,
            static_cast<float>(extent.height),
            static_cast<float>(extent.width),
            -static_cast<float>(extent.height),
            0.f,
            1.f
        };

        mCmd.setViewport(0, viewport);
    }

    void CommandRecorder::setScissor(vk::Extent2D extent)
    {
        vk::Rect2D scissor{{0, 0}, extent};
        mCmd.setScissor(0, scissor);
    }

    vk::CommandBuffer CommandRecorder::getCommandBuffer() const
    {
        return mCmd;
    }
}
