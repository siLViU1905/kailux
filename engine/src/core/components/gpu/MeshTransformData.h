#pragma once
#include "core/Core.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace kailux
{
    struct MeshTransformData
    {
        glm::vec4 position{0.f, 0.f, 0.f, 1.f};
        glm::quat rotation = glm::identity<glm::quat>();
        glm::vec4 scale{1.f, 1.f, 1.f, 0.f};

        glm::mat4 getModelMatrix() const
        {
            return glm::translate(glm::mat4(1.f), glm::vec3(position))
                   * glm::toMat4(rotation)
                   * glm::scale(glm::mat4(1.f), glm::vec3(scale));
        }
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(MeshTransformData)
}
