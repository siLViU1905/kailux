#pragma once
#include <glm/glm.hpp>

#include "../../Core.h"

namespace kailux
{
    struct CameraData
    {
        glm::mat4 projection{};
        glm::mat4 view{};
        glm::vec4 position{};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(CameraData)
}
