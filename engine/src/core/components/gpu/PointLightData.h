#pragma once
#include "../../Core.h"

namespace kailux
{
    struct PointLightData
    {
        glm::vec4 positionAndIntensity{ 0.f,0.f, 0.f, 1.f };
        glm::vec4 colorAndEnabled{ 1.f, 1.f, 1.f, 1.f };
        glm::vec4 range{ 10.f, 0.f, 0.f, 0.f };
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(PointLightData);
}