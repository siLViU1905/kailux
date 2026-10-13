#pragma once
#include "core/physics/PhysicsRegistry.h"

namespace kailux
{
    struct CachedPhysicsData
    {
        Shared<const PhysicsMeshData> mesh;
        MeshType                      meshType{MeshType::Unknown};
    };
}
