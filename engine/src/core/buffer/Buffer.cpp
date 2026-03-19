#include "Buffer.h"

namespace kailux
{
    Buffer::Buffer() : m_Buffer({}), m_Memory({}), m_Mapped(nullptr), m_Size(0)
    {
    }

    Buffer::Buffer(Buffer &&other) noexcept : m_Buffer(std::move(other.m_Buffer)),
                                              m_Memory(std::move(other.m_Memory)),
                                              m_Mapped(other.m_Mapped),
                                              m_Size(other.m_Size)
    {
        other.m_Mapped = nullptr;
        other.m_Size = 0;
    }

    Buffer::Buffer(vk::raii::Buffer &&buffer, vk::raii::DeviceMemory &&memory, void *mapped,
                   vk::DeviceSize size) : m_Buffer(std::move(buffer)),
                                          m_Memory(std::move(memory)),
                                          m_Mapped(mapped),
                                          m_Size(size)
    {
    }

    Buffer &Buffer::operator=(Buffer &&other) noexcept
    {
        if (this != &other)
        {
            m_Buffer = std::move(other.m_Buffer);
            m_Memory = std::move(other.m_Memory);
            m_Mapped = other.m_Mapped;
            m_Size = other.m_Size;

            other.m_Mapped = nullptr;
            other.m_Size = 0;
        }
        return *this;
    }

    Buffer::~Buffer()
    {
        if (m_Mapped && *m_Memory)
            m_Memory.unmapMemory();
    }

    void Buffer::upload(const void *data, vk::DeviceSize byte_size, vk::DeviceSize offset)
    {
        assert(m_Mapped && "Buffer is not host-visible");
        assert(offset + byte_size <= m_Size && "Upload exceeds buffer size");
        memcpy(static_cast<uint8_t *>(m_Mapped) + offset, data, byte_size);
    }
}
