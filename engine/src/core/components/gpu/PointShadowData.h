#pragma once
#include "core/Core.h"

namespace kailux
{
    struct PointShadowData
    {
        glm::vec4  positionAndFar{};

        // x = enabled, y = depth bias, z = normal offset, w = pcf radius
        glm::vec4  params{0.f, 0.0025f, 0.05f, 1.f};

        // index of the owning light inside LightsData::pointLights, ~0u when the slot is free
        glm::uvec4 lightIndex{~0u, 0, 0, 0};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(PointShadowData)

    struct PointShadowsData
    {
        std::array<PointShadowData, details::kMaxPointShadows> slots{};

        // x = number of occupied slots
        glm::uvec4                                             count{};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(PointShadowsData)
}
