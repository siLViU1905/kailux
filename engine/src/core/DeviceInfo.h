#pragma once

namespace kailux
{
    struct DeviceInfo
    {
        std::string deviceName{"Unknown"};
        std::string driverName{"Unknown"};
        std::string driverInfo;
        uint32_t    vramSizeMB{};
    };
}
