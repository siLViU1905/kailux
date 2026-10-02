#pragma once
#include "core/physics/PhysicsRegistry.h"

namespace kailux
{
    struct CachedPhysicsData
    {
        std::vector<SubmeshPhysicsInfo> submeshes;
        MeshType                        meshType{MeshType::Unknown};
    };
}
