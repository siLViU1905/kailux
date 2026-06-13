#pragma once
#include "core/physics/PhysicsRegistry.h"

namespace kailux
{
    struct PhysicsComponent
    {
        BodyHandle      handle;
        PhysicsBodyType type{PhysicsBodyType::Static};

        constexpr bool isDynamic() const {return type == PhysicsBodyType::Dynamic;}
    };
}
