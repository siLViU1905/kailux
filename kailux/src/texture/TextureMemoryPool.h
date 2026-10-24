#pragma once
#include <optional>
#include <vulkan/vulkan_raii.hpp>

#include "Core.h"
#include "utilities/FreeListZone.h"

namespace kailux
{
    class Context;

    class TextureMemoryPool
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(TextureMemoryPool)

        static TextureMemoryPool create(const Context& context, vk::DeviceSize size);

        std::optional<vk::DeviceSize> Alloc(const vk::MemoryRequirements& requirements);
        void                          Free(vk::DeviceSize offset);

        vk::DeviceMemory GetMemory() const;
        vk::DeviceSize   GetSize() const;

    private:
        void CreateMemory(const Context &context, vk::DeviceSize size);

        vk::raii::DeviceMemory mMemory{nullptr};
        uint32_t               mMemoryTypeIndex{~0u};
        FreeListZone           mZone;
    };
}