#include "BufferAllocator.h"

namespace kailux
{
    Buffer BufferAllocator::alloc_local(const Context &context, vk::DeviceSize size, vk::BufferUsageFlags usage)
    {
        return alloc(context,
                     size,
                     usage | vk::BufferUsageFlagBits::eTransferDst,
                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                     false
        );
    }

    Buffer BufferAllocator::alloc_host(const Context &context, vk::DeviceSize size, vk::BufferUsageFlags usage)
    {
        return alloc(context,
                     size,
                     usage,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     true
        );
    }

    Buffer BufferAllocator::alloc_vertex(const Context &context, vk::DeviceSize size)
    {
        return alloc_local(context, size, vk::BufferUsageFlagBits::eVertexBuffer);
    }

    Buffer BufferAllocator::alloc_index(const Context &context, vk::DeviceSize size)
    {
        return alloc_local(context, size, vk::BufferUsageFlagBits::eIndexBuffer);
    }

    Buffer BufferAllocator::alloc_staging(const Context &context, vk::DeviceSize size)
    {
        return alloc_host(context, size, vk::BufferUsageFlagBits::eTransferSrc);
    }

    Buffer BufferAllocator::alloc_uniform(const Context &context, vk::DeviceSize size)
    {
        return alloc_host(context, size, vk::BufferUsageFlagBits::eUniformBuffer);
    }

    Buffer BufferAllocator::alloc_storage(const Context &context, vk::DeviceSize size)
    {
        return alloc_host(context, size, vk::BufferUsageFlagBits::eStorageBuffer);
    }

    Buffer BufferAllocator::alloc(const Context &context, vk::DeviceSize size, vk::BufferUsageFlags usage,
                                  vk::MemoryPropertyFlags props, bool map)
    {
        auto buffer = vk::raii::Buffer(context.m_Device,
                                       vk::BufferCreateInfo{
                                           {},
                                           size,
                                           usage,
                                           vk::SharingMode::eExclusive
                                       });

        auto reqs = buffer.getMemoryRequirements();
        auto memory = vk::raii::DeviceMemory(context.m_Device,
                                             vk::MemoryAllocateInfo(
                                                 reqs.size,
                                                 context.findMemoryType(reqs.memoryTypeBits, props)
                                             )
        );
        buffer.bindMemory(*memory, 0);

        void *mapped = nullptr;
        if (map)
            mapped = memory.mapMemory(0, size);

        return {std::move(buffer), std::move(memory), mapped, size};
    }
}
