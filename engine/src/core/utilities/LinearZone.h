#pragma once

namespace kailux
{
    struct LinearZone
    {
        static constexpr vk::DeviceSize kDefaultAlignment = 16;

        vk::DeviceSize base{};
        vk::DeviceSize capacity{};
        vk::DeviceSize cursor{};

        vk::DeviceSize alloc(vk::DeviceSize size, vk::DeviceSize alignment = kDefaultAlignment);
    };
}