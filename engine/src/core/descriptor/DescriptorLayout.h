#pragma once
#include "../Core.h"
#include <vulkan/vulkan_raii.hpp>

#include "../Context.h"

namespace kailux
{
    struct DescriptorLayoutBinding
    {
        vk::DescriptorType      type{};
        uint32_t                count{};
        vk::ShaderStageFlagBits shaderType{};
    };

    class DescriptorLayout
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(DescriptorLayout)

        static DescriptorLayout create(const Context &context, std::span<DescriptorLayoutBinding> bindings);

        vk::DescriptorSetLayout getLayout() const;

    private:
        void createLayout(const Context &context, std::span<DescriptorLayoutBinding> bindings);

        vk::raii::DescriptorSetLayout m_Layout;
    };
}
