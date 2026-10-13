#pragma once

#include <glm/vec3.hpp>
#include <glm/detail/type_quat.hpp>

namespace kailux
{
    struct PhysicsInterpolation
    {
        glm::vec3 prevPosition{0.f};
        glm::vec3 currPosition{0.f};
        glm::quat prevRotation{1.f, 0.f, 0.f, 0.f};
        glm::quat currRotation{1.f, 0.f, 0.f, 0.f};
    };
}
