#include "CommandRecorder.h"

#include "Logger.h"

namespace kailux
{
    CommandRecorder::CommandRecorder(vk::CommandBuffer cmd):m_Cmd(cmd), m_InRendering(false), m_IsSecondary(false)
    {
        vk::CommandBufferBeginInfo beginInfo{
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        };

        m_Cmd.begin(beginInfo);
    }

    CommandRecorder::CommandRecorder(vk::CommandBuffer cmd,
        const vk::CommandBufferInheritanceRenderingInfo &inheritance): m_Cmd(cmd), m_InRendering(false), m_IsSecondary(true)
    {
        vk::CommandBufferInheritanceInfo inheritanceInfo;
        inheritanceInfo.pNext = &inheritance;

        vk::CommandBufferBeginInfo beginInfo{
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eRenderPassContinue,
            &inheritanceInfo
        };

        m_Cmd.begin(beginInfo);
    }

    CommandRecorder::~CommandRecorder()
    {
        if (m_InRendering && !m_IsSecondary)
            m_Cmd.endRendering();

        m_Cmd.end();
    }

    void CommandRecorder::barrier(const ImageBarrier &info) const
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
                vk::ImageAspectFlagBits::eColor,
                0, 1,
                0, 1
            }
        };

        vk::DependencyInfo depInfo{};
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers    = &imageBarrier;

        m_Cmd.pipelineBarrier2(depInfo);
    }

    void CommandRecorder::beginRendering(const RenderingInfo &info)
    {
        if (m_IsSecondary)
        {
            KAILUX_LOG_WARNING("[CommandRecorder]", "beginRendering() was called from a secondary buffer")
            return;
        }

        vk::RenderingAttachmentInfo colorAttachment{
            info.colorView,
            info.colorLayout,
            vk::ResolveModeFlagBits::eNone,
            {},
            {},
            info.loadOp,
            info.storeOp,
            vk::ClearValue{ info.clearColor }
        };

        vk::RenderingInfo renderingInfo{
                info.renderFlags,
                vk::Rect2D{ {0, 0}, info.extent },
                1,
                0,
                colorAttachment
            };

        vk::RenderingAttachmentInfo depthAttachment{};
        if (info.depthView)
        {
            depthAttachment = vk::RenderingAttachmentInfo{
                info.depthView,
                info.depthLayout,
                vk::ResolveModeFlagBits::eNone,
                {},
                {},
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::ClearValue{ vk::ClearDepthStencilValue{ 1.0f, 0 } }
            };

            renderingInfo.pDepthAttachment = &depthAttachment;
        }

        m_Cmd.beginRendering(renderingInfo);
        m_InRendering = true;
    }

    void CommandRecorder::endRendering()
    {
        if (!m_InRendering|| m_IsSecondary)
        {
            KAILUX_LOG_WARNING("[CommandRecorder]", "endRendering() was called while the command was rendering or from a secondary buffer")
            return;
        }

        m_Cmd.endRendering();
        m_InRendering = false;
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

        m_Cmd.setViewport(0, viewport);
    }

    void CommandRecorder::setScissor(vk::Extent2D extent)
    {
        vk::Rect2D scissor{ {0, 0}, extent };
        m_Cmd.setScissor(0, scissor);
    }

    vk::CommandBuffer CommandRecorder::getCommandBuffer() const
    {
        return m_Cmd;
    }
}
