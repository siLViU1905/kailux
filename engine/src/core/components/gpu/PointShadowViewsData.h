#pragma once
#include "PointShadowData.h"

namespace kailux
{
    struct PointShadowViewsData
    {
        std::array<PointShadowsData, details::kMaxCameraViews> views{};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(PointShadowViewsData)
}
