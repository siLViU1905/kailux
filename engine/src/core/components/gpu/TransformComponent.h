#pragma once
#include "MeshTransformData.h"

namespace kailux
{
    struct TransformComponent
    {
        MeshTransformData transform;
        glm::mat4         submeshLocalMatrix{1.f};
        glm::mat4         worldMatrix{1.f};
    };
}
