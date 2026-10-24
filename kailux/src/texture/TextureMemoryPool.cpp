#include "TextureMemoryPool.h"

#include "Context.h"
#include "Log.h"

namespace kailux
{
    TextureMemoryPool::TextureMemoryPool() = default;

    TextureMemoryPool::TextureMemoryPool(TextureMemoryPool &&other) noexcept : mMemory(std::move(other.mMemory)),
                                                                               mMemoryTypeIndex(other.mMemoryTypeIndex),
                                                                               mZone(std::move(other.mZone))
    {
    }

    TextureMemoryPool & TextureMemoryPool::operator=(TextureMemoryPool &&other) noexcept
    {
        if (this != &other)
        {
            mMemory = std::move(other.mMemory);
            mMemoryTypeIndex = other.mMemoryTypeIndex;
            mZone = std::move(other.mZone);
        }
        return *this;
    }

    TextureMemoryPool TextureMemoryPool::create(const Context &context, vk::DeviceSize size)
    {
        TextureMemoryPool pool;
        pool.CreateMemory(context, size);
        return pool;
    }

    std::optional<vk::DeviceSize> TextureMemoryPool::Alloc(const vk::MemoryRequirements &requirements)
    {
        if (!(requirements.memoryTypeBits & (1u << mMemoryTypeIndex)))
            return std::nullopt;

        return mZone.TryAlloc(requirements.size, requirements.alignment);
    }

    void TextureMemoryPool::Free(vk::DeviceSize offset)
    {
        mZone.Free(offset);
    }

    vk::DeviceMemory TextureMemoryPool::GetMemory() const
    {
        return *mMemory;
    }

    vk::DeviceSize TextureMemoryPool::GetSize() const
    {
        return mZone.GetSize();
    }

    void TextureMemoryPool::CreateMemory(const Context &context, vk::DeviceSize size)
    {
        vk::ImageCreateInfo probeInfo{};
        probeInfo.imageType = vk::ImageType::e2D;
        probeInfo.format = vk::Format::eR8G8B8A8Unorm;
        probeInfo.extent = vk::Extent3D{1, 1, 1};
        probeInfo.mipLevels = 1;
        probeInfo.arrayLayers = 1;
        probeInfo.samples = vk::SampleCountFlagBits::e1;
        probeInfo.tiling = vk::ImageTiling::eOptimal;
        probeInfo.usage = vk::ImageUsageFlagBits::eSampled
                          | vk::ImageUsageFlagBits::eTransferSrc
                          | vk::ImageUsageFlagBits::eTransferDst;
        probeInfo.sharingMode = vk::SharingMode::eExclusive;
        probeInfo.initialLayout = vk::ImageLayout::eUndefined;

        const vk::raii::Image probe{context.GetRAIIDevice(), probeInfo};
        const auto requirements{probe.getMemoryRequirements()};

        mMemoryTypeIndex = context.FindMemoryType(
            requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        mMemory = {
            context.GetRAIIDevice(),
            vk::MemoryAllocateInfo{size, mMemoryTypeIndex}
        };

        mZone = FreeListZone::create(0, size);

        log::console.Debug("texture memory pool: allocated {} MiB", size / (1024 * 1024));
    }
}
