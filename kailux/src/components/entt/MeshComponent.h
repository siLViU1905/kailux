#pragma once
#include "mesh/MeshRegistry.h"

namespace kailux
{
    struct MeshComponent
    {
        MeshHandle  handle;
        glm::vec4   boundingSphere{};
    };
}
