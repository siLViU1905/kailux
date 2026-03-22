#pragma once
#include "Buffer.h"
#include "core/Context.h"

namespace kailux
{
    class BufferAllocator
    {
    public:
        //device local -> for vertex, index buffers
        static Buffer alloc_local(const Context &context, vk::DeviceSize size, vk::BufferUsageFlags usage);

        //host visible + coherent -> for uniform, staging, indirect commands buffers
        static Buffer alloc_host(const Context &context, vk::DeviceSize size, vk::BufferUsageFlags usage);

        static Buffer alloc_vertex(const Context &context, vk::DeviceSize size);

        static Buffer alloc_index(const Context &context, vk::DeviceSize size);

        static Buffer alloc_staging(const Context &context, vk::DeviceSize size);

        static Buffer alloc_uniform(const Context &context, vk::DeviceSize size);

    private:
        static Buffer alloc(const Context &context, vk::DeviceSize size,
                            vk::BufferUsageFlags usage,
                            vk::MemoryPropertyFlags props,
                            bool map
        );
    };
}
