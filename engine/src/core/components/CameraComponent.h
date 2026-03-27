#pragma once
#include <glm/glm.hpp>

#include "core/Core.h"

namespace kailux
{
    struct CameraComponent
    {
        glm::mat4 projection{};
        glm::mat4 view{};
        glm::vec4 position{};
    };

    KAILUX_CHECK_COMPONENT_SIZE(CameraComponent)
}
