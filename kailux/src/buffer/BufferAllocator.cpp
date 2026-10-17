#include "BufferAllocator.h"
#include "Context.h"

namespace kailux::buffer_allocator
{
    Buffer alloc_local(const Context &context, vk::DeviceSize size, vk::BufferUsageFlags usage)
    {
        return details::alloc(context,
                              size,
                              usage | vk::BufferUsageFlagBits::eTransferDst,
                              vk::MemoryPropertyFlagBits::eDeviceLocal,
                              false
        );
    }

    Buffer alloc_host(const Context &context, vk::DeviceSize size, vk::BufferUsageFlags usage)
    {
        return details::alloc(context,
                              size,
                              usage,
                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                              true
        );
    }

    Buffer alloc_vertex(const Context &context, vk::DeviceSize size)
    {
        return alloc_local(context, size, vk::BufferUsageFlagBits::eVertexBuffer);
    }

    Buffer alloc_index(const Context &context, vk::DeviceSize size)
    {
        return alloc_local(context, size, vk::BufferUsageFlagBits::eIndexBuffer);
    }

    Buffer alloc_staging(const Context &context, vk::DeviceSize size)
    {
        return alloc_host(context, size, vk::BufferUsageFlagBits::eTransferSrc);
    }

    Buffer alloc_uniform(const Context &context, vk::DeviceSize size)
    {
        return alloc_host(context, size, vk::BufferUsageFlagBits::eUniformBuffer);
    }

    Buffer alloc_storage(const Context &context, vk::DeviceSize size)
    {
        return alloc_host(context, size, vk::BufferUsageFlagBits::eStorageBuffer);
    }

    Buffer details::alloc(const Context &context, vk::DeviceSize size, vk::BufferUsageFlags usage,
                                  vk::MemoryPropertyFlags props, bool map)
    {
        auto buffer = vk::raii::Buffer(context.GetRAIIDevice(),
                                       vk::BufferCreateInfo{
                                           {},
                                           size,
                                           usage,
                                           vk::SharingMode::eExclusive
                                       });

        auto reqs = buffer.getMemoryRequirements();
        auto memory = vk::raii::DeviceMemory(context.GetRAIIDevice(),
                                             vk::MemoryAllocateInfo(
                                                 reqs.size,
                                                 context.FindMemoryType(reqs.memoryTypeBits, props)
                                             )
        );
        buffer.bindMemory(*memory, 0);

        void *mapped = nullptr;
        if (map)
            mapped = memory.mapMemory(0, size);

        return {std::move(buffer), std::move(memory), mapped, size};
    }
}
