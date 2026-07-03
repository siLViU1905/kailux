#include "DescriptorLayout.h"

#include "../Log.h"

namespace kailux
{
    DescriptorLayout::DescriptorLayout() : mLayout({})
    {
    }

    DescriptorLayout::DescriptorLayout(DescriptorLayout &&other) noexcept : mLayout(std::move(other.mLayout))
    {
    }

    DescriptorLayout &DescriptorLayout::operator=(DescriptorLayout &&other) noexcept
    {
        if (this != &other)
        {
            mLayout = std::move(other.mLayout);
        }
        return *this;
    }

    DescriptorLayout DescriptorLayout::create(const Context &context,
                                                    std::span<const DescriptorLayoutBinding> bindings)
    {
        KAILUX_LOG_PARENT_CLR_GREEN("[DescriptorSetLayout]")
        DescriptorLayout layout;

        layout.createLayout(context, bindings);
        KAILUX_LOG_CHILD_CLR_GREEN(std::format("Created descriptor layout with {} bindings", bindings.size()))

        return layout;
    }

    vk::DescriptorSetLayout DescriptorLayout::getLayout() const
    {
        return *mLayout;
    }

    void DescriptorLayout::createLayout(const Context &context, std::span<const DescriptorLayoutBinding> bindings)
    {
        std::vector<vk::DescriptorSetLayoutBinding> vkBindings;
        std::vector<vk::DescriptorBindingFlags> bindingFlagsArr;
        vkBindings.reserve(bindings.size());
        bindingFlagsArr.reserve(bindings.size());
        uint32_t bindPoint = 0;
        for (auto bind: bindings)
        {
            vkBindings.emplace_back(
                bindPoint,
                bind.type,
                bind.count,
                bind.shaderType
            );

            vk::DescriptorBindingFlags f = vk::DescriptorBindingFlagBits::ePartiallyBound;

            if (bind.type == vk::DescriptorType::eCombinedImageSampler)
                f |= vk::DescriptorBindingFlagBits::eUpdateAfterBind;

            bindingFlagsArr.push_back(f);
            ++bindPoint;
        }

        vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlags;
        bindingFlags.bindingCount = static_cast<uint32_t>(bindingFlagsArr.size());
        bindingFlags.pBindingFlags = bindingFlagsArr.data();

        vk::DescriptorSetLayoutCreateInfo layoutInfo(
            vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
            static_cast<uint32_t>(vkBindings.size()),
            vkBindings.data()
        );
        layoutInfo.pNext = &bindingFlags;

        mLayout = vk::raii::DescriptorSetLayout(context.mDevice, layoutInfo);
    }
}
