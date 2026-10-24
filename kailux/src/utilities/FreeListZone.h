#pragma once

#include <list>
#include <optional>
#include <vulkan/vulkan_raii.hpp>

namespace kailux
{
    class FreeListZone
    {
    public:
        static FreeListZone create(vk::DeviceSize base, vk::DeviceSize capacity);

        vk::DeviceSize Alloc(vk::DeviceSize size, vk::DeviceSize alignment = kDefaultAlignment);

        std::optional<vk::DeviceSize> TryAlloc(vk::DeviceSize size, vk::DeviceSize alignment = kDefaultAlignment);

        void Free(vk::DeviceSize offset);

    private:
        static constexpr vk::DeviceSize kDefaultAlignment{16};

        struct Block
        {
            vk::DeviceSize offset{};
            vk::DeviceSize size{};
        };

        vk::DeviceSize mBase{};
        vk::DeviceSize mCapacity{};

        std::list<Block> mFreeBlocks;
        std::list<Block> mUsedBlocks;
    };
}