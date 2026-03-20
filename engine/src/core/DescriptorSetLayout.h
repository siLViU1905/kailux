#pragma once
#include "Core.h"
#include <vulkan/vulkan_raii.hpp>

#include "Context.h"

namespace kailux
{
    struct DescriptorSetLayoutBinding
    {
        vk::DescriptorType      type{};
        uint32_t                count{};
        vk::ShaderStageFlagBits shaderType{};
    };

    class DescriptorSetLayout
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(DescriptorSetLayout)

        static DescriptorSetLayout create(const Context &context, std::span<DescriptorSetLayoutBinding> bindings);

        vk::DescriptorSetLayout getLayout() const;

    private:
        void createLayout(const Context &context, std::span<DescriptorSetLayoutBinding> bindings);

        vk::raii::DescriptorSetLayout m_Layout;
    };
}
