#include "Buffer.h"

namespace kailux
{
    Buffer::Buffer() : mBuffer({}), mMemory({}), mMapped(nullptr), mSize(0)
    {
    }

    Buffer::Buffer(Buffer &&other) noexcept : mBuffer(std::move(other.mBuffer)),
                                              mMemory(std::move(other.mMemory)),
                                              mMapped(other.mMapped),
                                              mSize(other.mSize)
    {
        other.mMapped = nullptr;
        other.mSize = 0;
    }

    Buffer::Buffer(vk::raii::Buffer &&buffer, vk::raii::DeviceMemory &&memory, void *mapped,
                   vk::DeviceSize size) : mBuffer(std::move(buffer)),
                                          mMemory(std::move(memory)),
                                          mMapped(mapped),
                                          mSize(size)
    {
    }

    Buffer &Buffer::operator=(Buffer &&other) noexcept
    {
        if (this != &other)
        {
            mBuffer = std::move(other.mBuffer);
            mMemory = std::move(other.mMemory);
            mMapped = other.mMapped;
            mSize = other.mSize;

            other.mMapped = nullptr;
            other.mSize = 0;
        }
        return *this;
    }

    Buffer::~Buffer()
    {
        if (mMapped && *mMemory)
            mMemory.unmapMemory();
    }

    vk::Buffer Buffer::getBuffer() const
    {
        return *mBuffer;
    }

    vk::DeviceSize Buffer::getSize() const
    {
        return mSize;
    }

    void Buffer::upload(const void *data, vk::DeviceSize byte_size, vk::DeviceSize offset) const
    {
        assert(mMapped && "Buffer is not host-visible");
        assert(offset + byte_size <= mSize && "Upload exceeds buffer size");
        memcpy(static_cast<uint8_t *>(mMapped) + offset, data, byte_size);
    }
}
