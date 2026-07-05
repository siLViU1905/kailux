#include "LinearZone.h"

namespace kailux
{
    vk::DeviceSize LinearZone::alloc(vk::DeviceSize size, vk::DeviceSize alignment)
    {
        vk::DeviceSize aligned = ((cursor + alignment - 1) / alignment) * alignment;
        if (aligned + size > capacity)
            throw std::runtime_error("Built in mesh zone out of memory");
        cursor = aligned + size;
        return base + aligned;
    }
}
