#pragma once
#include "core/physics/PhysicsRegistry.h"

namespace kailux
{
    struct PhysicsComponent
    {
        BodyHandle      handle;
        PhysicsBodyType type{PhysicsBodyType::Static};
        bool            canBecomeDynamic{false};

        constexpr bool IsDynamic() const {return type == PhysicsBodyType::Dynamic;}
    };
}
