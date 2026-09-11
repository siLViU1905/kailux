#pragma once
#include "core/Core.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace kailux
{
    using ModelMatrixType = glm::mat4;

    struct Transform
    {
        glm::vec3 position{0.f};
        glm::quat rotation{glm::identity<glm::quat>()};
        glm::vec3 scale{1.f};

        glm::mat4 GetModelMatrix() const;

        static Transform from_matrix(const glm::mat4 &model);
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(ModelMatrixType)
}
