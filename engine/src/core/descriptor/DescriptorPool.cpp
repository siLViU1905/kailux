#include "DescriptorPool.h"

namespace kailux
{
    DescriptorPool::DescriptorPool() : mPool({})
    {
    }

    DescriptorPool::DescriptorPool(DescriptorPool &&other) noexcept : mPool(std::move(other.mPool))
    {
    }

    DescriptorPool &DescriptorPool::operator=(DescriptorPool &&other) noexcept
    {
        if (this != &other)
        {
            mPool = std::move(other.mPool);
        }
        return *this;
    }

    DescriptorPool DescriptorPool::create(const Context &context, uint32_t sets,
                                          std::span<const DescriptorPoolSize> sizes)
    {
        DescriptorPool pool;
        pool.createPool(context, sets, sizes);
        return pool;
    }

    vk::DescriptorPool DescriptorPool::getPool() const
    {
        return *mPool;
    }

    void DescriptorPool::createPool(const Context &context, uint32_t sets, std::span<const DescriptorPoolSize> sizes)
    {
        std::vector<vk::DescriptorPoolSize> poolSizes;
        poolSizes.reserve(sizes.size());
        for (auto [type, count]: sizes)
            poolSizes.emplace_back(type, sets * count);

        vk::DescriptorPoolCreateInfo createInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
            sets,
            poolSizes
        );
        mPool = vk::raii::DescriptorPool(context.mDevice, createInfo);
    }
}
