#include "DescriptorPool.h"

namespace kailux
{
    DescriptorPool::DescriptorPool() : m_Pool({})
    {
    }

    DescriptorPool::DescriptorPool(DescriptorPool &&other) noexcept : m_Pool(std::move(other.m_Pool))
    {
    }

    DescriptorPool &DescriptorPool::operator=(DescriptorPool &&other) noexcept
    {
        if (this != &other)
        {
            m_Pool = std::move(other.m_Pool);
        }
        return *this;
    }

    DescriptorPool DescriptorPool::create(const Context &context, uint32_t sets,
                                          std::span<DescriptorPoolSize> sizes)
    {
        DescriptorPool pool;
        pool.createPool(context, sets, sizes);
        return pool;
    }

    vk::DescriptorPool DescriptorPool::getPool() const
    {
        return *m_Pool;
    }

    void DescriptorPool::createPool(const Context &context, uint32_t sets, std::span<DescriptorPoolSize> sizes)
    {
        std::vector<vk::DescriptorPoolSize> poolSizes;
        poolSizes.reserve(sizes.size());
        for (auto [type, count]: sizes)
            poolSizes.emplace_back(type, sets * count);

        vk::DescriptorPoolCreateInfo createInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            sets,
            poolSizes
        );
        m_Pool = vk::raii::DescriptorPool(context.m_Device, createInfo);
    }
}
