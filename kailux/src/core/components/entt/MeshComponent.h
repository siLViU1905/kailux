#pragma once
#include "core/mesh/MeshRegistry.h"

namespace kailux
{
    struct MeshComponent
    {
        MeshHandle  handle;
        glm::vec4   boundingSphere{};
    };
}
