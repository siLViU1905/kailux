#pragma once
#include "DirectionalLightData.h"
#include "DirectionalShadowData.h"
#include "LightsData.h"
#include "PointShadowData.h"
#include "core/Core.h"

namespace kailux
{
    struct SceneData
    {
        LightsData lights;
        DirectionalShadowData directionalShadow;
        PointShadowsData pointShadows;
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(SceneData)
}
