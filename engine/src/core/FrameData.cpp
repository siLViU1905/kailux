#include "FrameData.h"

#include "Pipeline.h"
#include "buffer/BufferAllocator.h"
#include "components/CameraComponent.h"

namespace kailux
{
    FrameData::FrameData() : m_CommandPool({}),
                             m_ImGuiCommandPool({}),
                             m_CommandBuffer({}),
                             m_ImGuiCommandBuffer({}),
                             m_FenceInFlight({})

    {
    }

    FrameData::FrameData(FrameData &&other) noexcept : m_CommandPool(std::move(other.m_CommandPool)),
                                                       m_ImGuiCommandPool(std::move(other.m_ImGuiCommandPool)),
                                                       m_CommandBuffer(std::move(other.m_CommandBuffer)),
                                                       m_ImGuiCommandBuffer(std::move(other.m_ImGuiCommandBuffer)),
                                                       m_FenceInFlight(std::move(other.m_FenceInFlight)),
                                                       m_DescriptorSet(std::move(other.m_DescriptorSet)),
                                                       m_CameraBuffer(std::move(other.m_CameraBuffer)),
                                                       m_IndirectBuffer(std::move(other.m_IndirectBuffer))
    {
    }

    FrameData &FrameData::operator=(FrameData &&other) noexcept
    {
        if (this != &other)
        {
            m_CommandPool = std::move(other.m_CommandPool);
            m_ImGuiCommandPool = std::move(other.m_ImGuiCommandPool);
            m_CommandBuffer = std::move(other.m_CommandBuffer);
            m_ImGuiCommandBuffer = std::move(other.m_ImGuiCommandBuffer);
            m_FenceInFlight = std::move(other.m_FenceInFlight);
            m_DescriptorSet = std::move(other.m_DescriptorSet);
            m_CameraBuffer = std::move(other.m_CameraBuffer);
            m_IndirectBuffer = std::move(other.m_IndirectBuffer);
        }
        return *this;
    }

    FrameData FrameData::create(const Context &context, const DescriptorLayout &descriptorLayout,
                                const DescriptorPool &descriptorPool, uint32_t maxMeshCount)
    {
        FrameData frame;
        frame.createCommandPool(context);
        frame.createImGuiCommandPool(context);
        frame.createCommandBuffer(context);
        frame.createImGuiCommandBuffer(context);
        frame.createSyncObjects(context);
        frame.createCameraBuffer(context);
        frame.createIndirectBuffer(context, maxMeshCount);
        auto descSetInfo = frame.makeDescriptorSetInfo();
        frame.createDescriptorSet(context, descriptorLayout, descriptorPool, descSetInfo);
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

    vk::CommandBuffer FrameData::getImGuiCommandBuffer() const
    {
        return *m_ImGuiCommandBuffer;
    }

    vk::Fence FrameData::getFenceInFlight() const
    {
        return *m_FenceInFlight;
    }

    const DescriptorSet &FrameData::getDescriptorSet() const
    {
        return m_DescriptorSet;
    }

    Buffer &FrameData::getCameraBuffer()
    {
        return m_CameraBuffer;
    }

    Buffer & FrameData::getIndirectBuffer()
    {
        return m_IndirectBuffer;
    }

    void FrameData::createCommandPool(const Context &context)
    {
        vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        m_CommandPool = vk::raii::CommandPool(context.m_Device, poolInfo);
    }

    void FrameData::createImGuiCommandPool(const Context &context)
    {
        vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        m_ImGuiCommandPool = vk::raii::CommandPool(context.m_Device, poolInfo);
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

    void FrameData::createImGuiCommandBuffer(const Context &context)
    {
        vk::CommandBufferAllocateInfo allocInfo(
            m_CommandPool,
            vk::CommandBufferLevel::eSecondary,
            1
        );

        m_ImGuiCommandBuffer = std::move(vk::raii::CommandBuffers(context.m_Device, allocInfo).front());
    }

    void FrameData::createSyncObjects(const Context &context)
    {
        m_FenceInFlight = vk::raii::Fence(context.m_Device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }

    void FrameData::createDescriptorSet(const Context &context, const DescriptorLayout &descriptorLayout,
                                        const DescriptorPool &descriptorPool, std::span<DescriptorSetInfo> infos)
    {
        m_DescriptorSet = DescriptorSet::create(context, descriptorLayout, descriptorPool, infos);
    }

    void FrameData::createCameraBuffer(const Context &context)
    {
        m_CameraBuffer = BufferAllocator::alloc_uniform(context, sizeof(CameraComponent));
    }

    void FrameData::createIndirectBuffer(const Context &context, uint32_t count)
    {
        m_IndirectBuffer = BufferAllocator::alloc_host(context, count * sizeof(vk::DrawIndexedIndirectCommand), vk::BufferUsageFlagBits::eIndirectBuffer);
    }

    std::array<DescriptorSetInfo, 1> FrameData::makeDescriptorSetInfo() const
    {
        return {
            {
                DescriptorSetBufferInfo(
                    vk::DescriptorType::eUniformBuffer,
                    m_CameraBuffer.getBuffer(),
                    sizeof(CameraComponent),
                    1
                )
            }
        };
    }
}
