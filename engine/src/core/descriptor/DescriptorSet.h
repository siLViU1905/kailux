#pragma once
#include "../Core.h"
#include "../Context.h"
#include "DescriptorLayout.h"
#include "DescriptorPool.h"
#include "../Pipeline.h"

namespace kailux
{
    struct DescriptorSetBufferInfo
    {
        vk::DescriptorType type{};
        vk::Buffer buffer{};
        vk::DeviceSize size{};
        uint32_t count{};
    };

    struct DescriptorSetImageInfo
    {
        vk::Sampler sampler{};
        vk::ImageView view{};
        vk::ImageLayout layout{};
        uint32_t count{};
    };

    using DescriptorSetInfo = std::variant<DescriptorSetBufferInfo, DescriptorSetImageInfo>;

    struct DescriptorSetUpdateInfo
    {
        uint32_t          binding{};
        uint32_t          arrayIndex{};
        DescriptorSetInfo info;
    };

    class DescriptorSet
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(DescriptorSet)

        static DescriptorSet create(const Context &context, const DescriptorLayout &layout, const DescriptorPool &pool,
                                    std::span<const DescriptorSetInfo> infos);

        vk::DescriptorSet getDescriptorSet() const;

        void bind(const Pipeline& pipeline, vk::CommandBuffer cmd) const;
        void updateInfo(const Context& context, std::span<const DescriptorSetUpdateInfo> updateInfos) const;

    private:
        void createSet(const Context &context, const DescriptorLayout &layout, const DescriptorPool &pool,
                       std::span<const DescriptorSetInfo> infos);

        vk::raii::DescriptorSet m_Set;
    };
}
