#pragma once
#include "DirectionalLightData.h"
#include "DirectionalShadowData.h"
#include "LightsData.h"
#include "core/Core.h"

namespace kailux
{
    struct SceneData
    {
        LightsData lights;
        DirectionalShadowData directionalShadow;
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(SceneData)
}
