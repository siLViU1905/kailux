#pragma once
#include <vulkan/vulkan_raii.hpp>
#include "../Core.h"

namespace kailux
{
    class Buffer
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Buffer)
        Buffer(vk::raii::Buffer&& buffer, vk::raii::DeviceMemory&& memory, void* mapped, vk::DeviceSize size);
        ~Buffer();

        vk::Buffer getBuffer() const;

        void upload(const void* data, vk::DeviceSize byte_size, vk::DeviceSize offset = 0);
        template<typename T>
        void upload(std::span<const T> data, vk::DeviceSize offset = 0)
        {
            upload(data.data(), data.size_bytes(), offset);
        }

    private:
        vk::raii::Buffer       m_Buffer;
        vk::raii::DeviceMemory m_Memory;
        void*                  m_Mapped;
        vk::DeviceSize         m_Size;
    };
}
