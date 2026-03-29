#pragma once
#include "core/Core.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace kailux
{
    using ModelMatrixType = glm::mat4;

    struct MeshTransformData
    {
        glm::vec3 position{0.f};
        glm::quat rotation = glm::identity<glm::quat>();
        glm::vec3 scale{1.f};

        ModelMatrixType getModelMatrix() const
        {
            return glm::translate(glm::mat4(1.f), position)
                   * glm::toMat4(rotation)
                   * glm::scale(glm::mat4(1.f), scale);
        }
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(ModelMatrixType)
}
