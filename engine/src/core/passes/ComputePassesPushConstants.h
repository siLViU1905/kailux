#pragma once

namespace kailux
{
    struct ComputePassesPushConstants
    {
        struct MouseCords
        {
            uint32_t x{};
            uint32_t y{};
        };

        struct CameraFrustum
        {
            std::array<glm::vec4, 6> frustumPlanes{};
            uint32_t                 totalObjects{};
        };
    };
}
