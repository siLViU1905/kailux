#pragma once
#include "Core.h"

namespace kailux
{
    struct ShadowCascadeData
    {
        glm::mat4 viewProjection{1.f};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(ShadowCascadeData)
}
