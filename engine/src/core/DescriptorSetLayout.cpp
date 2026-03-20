#include "DescriptorSetLayout.h"

#include "Logger.h"

namespace kailux
{
    DescriptorSetLayout::DescriptorSetLayout() : m_Layout({})
    {
    }

    DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout &&other) noexcept : m_Layout(std::move(other.m_Layout))
    {
    }

    DescriptorSetLayout &DescriptorSetLayout::operator=(DescriptorSetLayout &&other) noexcept
    {
        if (this != &other)
        {
            m_Layout = std::move(other.m_Layout);
        }
        return *this;
    }

    DescriptorSetLayout DescriptorSetLayout::create(const Context &context,
                                                    std::span<DescriptorSetLayoutBinding> bindings)
    {
        KAILUX_LOG_PARENT_CLR_GREEN("[DescriptorSetLayout]")
        DescriptorSetLayout layout;

        layout.createLayout(context, bindings);
        KAILUX_LOG_CHILD_CLR_GREEN(std::format("Created descriptor set layout with {} bindings", bindings.size()))

        return layout;
    }

    vk::DescriptorSetLayout DescriptorSetLayout::getLayout() const
    {
        return *m_Layout;
    }

    void DescriptorSetLayout::createLayout(const Context &context, std::span<DescriptorSetLayoutBinding> bindings)
    {
        std::vector<vk::DescriptorSetLayoutBinding> vkBindings;
        vkBindings.reserve(bindings.size());
        uint32_t bindPoint = 0;
        for (auto bind: bindings)
        {
            vkBindings.emplace_back(
                bindPoint,
                bind.type,
                bind.count,
                bind.shaderType
            );
            ++bindPoint;
        }

        vk::DescriptorSetLayoutCreateInfo layoutInfo(
            {},
            static_cast<uint32_t>(vkBindings.size()),
            vkBindings.data()
        );

        m_Layout = vk::raii::DescriptorSetLayout(context.m_Device, layoutInfo);
    }
}
