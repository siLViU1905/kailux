#pragma once

#include <entt/entt.hpp>
#include "physics/PhysicsRegistry.h"

namespace kailux
{
    struct PhysicsComponent
    {
        BodyHandle      handle;
        PhysicsBodyType type{PhysicsBodyType::Static};
        bool            canBecomeDynamic{false};
        float           mass{};
        bool            isSensor{};

        constexpr bool IsDynamic() const {return type == PhysicsBodyType::Dynamic;}
    };

    constexpr uint64_t to_body_user_data(entt::entity entity)
    {
        return static_cast<uint64_t>(entt::to_integral(entity)) + 1u;
    }

    constexpr entt::entity from_body_user_data(uint64_t userData)
    {
        return userData == 0u ? entt::null : static_cast<entt::entity>(userData - 1u);
    }
}
