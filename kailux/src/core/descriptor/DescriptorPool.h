#pragma once
#include "DescriptorLayout.h"
#include "../Context.h"
#include "../Core.h"

namespace kailux
{
    struct DescriptorPoolSize
    {
        vk::DescriptorType type{};
        uint32_t count{};
    };

    class DescriptorPool
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(DescriptorPool)

        static DescriptorPool create(const Context& context, uint32_t sets, std::span<const DescriptorPoolSize> sizes);

        vk::DescriptorPool GetPool() const;

    private:
        void CreatePool(const Context& context, uint32_t sets, std::span<const DescriptorPoolSize> sizes);

        vk::raii::DescriptorPool mPool;
    };
}
