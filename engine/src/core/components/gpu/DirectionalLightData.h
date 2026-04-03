#pragma once
#include "core/Core.h"

namespace kailux
{
    struct DirectionalLightData
    {
        glm::vec4 directionAndIntensity{ 0.f, -1.f, 0.f, 2.5f };
        glm::vec4 colorAndEnabled{ 1.f, 1.f, 1.f, 1.f };
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(DirectionalLightData)
}
