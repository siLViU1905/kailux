#pragma once

namespace kailux
{
    struct GraphicsPassesPushConstants
    {
        struct Outline
        {
            glm::vec4               color{};
            uint32_t                id = ~0u;
            std::array<uint32_t, 3> _padding;
        };

        struct Gizmo
        {
            glm::vec4 positionAndScale{};
            glm::vec4 color{};
        };
    };
}
