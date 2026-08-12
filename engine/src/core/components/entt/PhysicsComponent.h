#pragma once
#include "core/physics/PhysicsRegistry.h"

namespace kailux
{
    struct PhysicsComponent
    {
        BodyHandle      handle;
        PhysicsBodyType type{PhysicsBodyType::Static};
        bool            canBecomeDynamic{false};

        constexpr bool isDynamic() const {return type == PhysicsBodyType::Dynamic;}
    };
}
