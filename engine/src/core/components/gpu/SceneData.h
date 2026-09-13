#pragma once

#include "DirectionalShadowsData.h"
#include "LightsData.h"
#include "PointShadowData.h"
#include "core/Core.h"

namespace kailux
{
    struct SceneData
    {
        LightsData lights;
        DirectionalShadowsData directionalShadows;
        PointShadowsData pointShadows;
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(SceneData)
}
