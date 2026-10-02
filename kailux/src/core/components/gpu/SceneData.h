#pragma once

#include "DirectionalShadowsData.h"
#include "LightsData.h"
#include "PointShadowViewsData.h"
#include "core/Core.h"

namespace kailux
{
    struct SceneData
    {
        LightsData lights;
        DirectionalShadowsData directionalShadows;
        PointShadowViewsData pointShadows;
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(SceneData)
}
