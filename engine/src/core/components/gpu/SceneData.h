#pragma once
#include "DirectionalLightData.h"
#include "LightsData.h"
#include "core/Core.h"

namespace kailux
{
    struct SceneData
    {
        LightsData lights;
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(SceneData)
}
