#pragma once
#include <glm/glm.hpp>

#include "../../Core.h"

namespace kailux
{
    struct CameraData
    {
        static constexpr float s_DefaultExposure = 0.0001f;

        glm::mat4 projection{};
        glm::mat4 view{};
        glm::vec4 positionAndExposure{0.f, 0.f, 0.f, s_DefaultExposure};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(CameraData)
}
