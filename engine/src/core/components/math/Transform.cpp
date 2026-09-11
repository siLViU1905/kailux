#include "Transform.h"
#include <glm/gtx/matrix_decompose.hpp>

namespace kailux
{
    glm::mat4 Transform::GetModelMatrix() const
    {
        return glm::translate(glm::mat4(1.f), position)
                   * glm::toMat4(rotation)
                   * glm::scale(glm::mat4(1.f), scale);
    }

    Transform Transform::from_matrix(const glm::mat4 &model)
    {
        Transform t;
        glm::vec3 skew; glm::vec4 persp;
        glm::decompose(model, t.scale, t.rotation, t.position, skew, persp);
        return t;
    }
}
