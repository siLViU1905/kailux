#pragma once
#include "Buffer.h"

namespace kailux
{
    class Context;

    namespace buffer_allocator
    {
        //device local -> for vertex, index buffers
        Buffer alloc_local(const Context &context, vk::DeviceSize size, vk::BufferUsageFlags usage);

        //host visible + coherent -> for uniform, staging, indirect buffers
        Buffer alloc_host(const Context &context, vk::DeviceSize size, vk::BufferUsageFlags usage);

        Buffer alloc_vertex(const Context &context, vk::DeviceSize size);

        Buffer alloc_index(const Context &context, vk::DeviceSize size);

        Buffer alloc_staging(const Context &context, vk::DeviceSize size);

        Buffer alloc_uniform(const Context &context, vk::DeviceSize size);

        Buffer alloc_storage(const Context &context, vk::DeviceSize size);

        namespace details
        {
            Buffer alloc(const Context &context, vk::DeviceSize size,
                                vk::BufferUsageFlags usage,
                                vk::MemoryPropertyFlags props,
                                bool map
            );
        }
    };
}
