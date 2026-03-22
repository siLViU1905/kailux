#pragma once
#include "DescriptorLayout.h"
#include "../Context.h"
#include "../Core.h"

namespace kailux
{
    class DescriptorPool
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(DescriptorPool)

        static DescriptorPool create(const Context& context, uint32_t sets, std::span<DescriptorLayoutBinding> bindings);

        vk::DescriptorPool getPool() const;

    private:
        void createPool(const Context& context, uint32_t sets, std::span<DescriptorLayoutBinding> bindings);

        vk::raii::DescriptorPool m_Pool;
    };
}
