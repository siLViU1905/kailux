#pragma once
#include "DirectionalLightData.h"
#include "core/Core.h"

namespace kailux
{
    struct SceneData
    {
        DirectionalLightData directionalLight;
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(SceneData)
}
