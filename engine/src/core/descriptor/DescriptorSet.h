#pragma once
#include "../Core.h"
#include "../Context.h"

namespace kailux
{
    struct DescriptorBufferInfo
    {
        vk::DescriptorType type{};
        vk::Buffer buffer{};
        vk::DeviceSize size{};
        uint32_t count{};
    };

    struct DescriptorImageInfo
    {
        vk::Sampler sampler{};
        vk::ImageView view{};
        vk::ImageLayout layout{};
        uint32_t count{};
    };

    using DescriptorInfo = std::variant<DescriptorBufferInfo, DescriptorImageInfo>;

    class DescriptorSet
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(DescriptorSet)

        static DescriptorSet create(const Context &context, const DescriptorLayout &layout, const DescriptorPool &pool,
                                    std::span<DescriptorInfo> infos);

    private:
        void createSet(const Context &context, const DescriptorLayout &layout, const DescriptorPool &pool,
                       std::span<DescriptorInfo> infos);

        vk::raii::DescriptorSet m_Set;
    };
}
