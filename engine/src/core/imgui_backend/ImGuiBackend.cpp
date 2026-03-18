#include "ImGuiBackend.h"
#include "../Logger.h"

namespace kailux
{
    ImGuiBackend::ImGuiBackend() : m_DescriptorPool({})
    {
    }

    ImGuiBackend::ImGuiBackend(ImGuiBackend &&other) noexcept : m_DescriptorPool(std::move(other.m_DescriptorPool))
    {
    }

    ImGuiBackend &ImGuiBackend::operator=(ImGuiBackend &&other) noexcept
    {
        if (this != &other)
        {
            m_DescriptorPool = std::move(other.m_DescriptorPool);
        }
        return *this;
    }

    ImGuiBackend ImGuiBackend::create(const Context &context)
    {
        KAILUX_LOG_PARENT_CLR_MAGENTA("[IMGUI BACKEND]")
        ImGuiBackend imguiBackend;

        imguiBackend.createDescriptorPool(context);
        KAILUX_LOG_CHILD_CLR_MAGENTA("Descriptor pool created")

        return imguiBackend;
    }

    void ImGuiBackend::createDescriptorPool(const Context &context)
    {
        constexpr uint32_t descriptorCount = 1000;

        constexpr std::array poolSizes = {
            vk::DescriptorPoolSize(vk::DescriptorType::eSampler, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, descriptorCount)
        };

        vk::DescriptorPoolCreateInfo poolInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            descriptorCount * poolSizes.size(),
            poolSizes.size(),
            poolSizes.data()
        );

        m_DescriptorPool = vk::raii::DescriptorPool(context.m_Device, poolInfo);
    }
}
