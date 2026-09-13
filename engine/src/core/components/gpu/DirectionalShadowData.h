#pragma once
#include "ShadowCascadeData.h"

namespace kailux
{
    struct DirectionalShadowData
    {
        std::array<ShadowCascadeData, details::kShadowCascadeCount> cascades{};

        glm::vec4 splitDepths{}; // far view space depth of every cascade, packed per component

        glm::vec4 params{1.f, 0.0015f, 0.05f, 1.f}; // x = enabled, y = depth bias, z = normal offset, w = pcf radius
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(DirectionalShadowData)
}
