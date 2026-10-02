#pragma once
#include "DirectionalLightData.h"
#include "PointLightData.h"
#include "core/Core.h"

namespace kailux
{
    struct LightsData
    {
        DirectionalLightData                                 directional;

        std::array<PointLightData, details::kMaxPointLights> pointLights;
        uint32_t                                             pointLightCount{};
        std::array<uint32_t, 3>                              _padding{};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(LightsData)
}
