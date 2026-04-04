#pragma once
#include "DirectionalLightData.h"
#include "core/Core.h"

namespace kailux
{
    struct SceneData
    {
        DirectionalLightData directionalLight;
        glm::vec4            ambient{0.5f, 0.7f, 1.f, 0.5f};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(SceneData)
}
