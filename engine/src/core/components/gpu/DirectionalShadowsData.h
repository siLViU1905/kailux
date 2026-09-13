#pragma once
#include "DirectionalShadowData.h"

namespace kailux
{
    struct DirectionalShadowsData
    {
        std::array<DirectionalShadowData, details::kMaxCameraViews> views{};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(DirectionalShadowsData)
}
