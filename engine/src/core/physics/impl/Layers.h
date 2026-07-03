#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>


namespace kailux
{
    namespace layers
    {
        static constexpr JPH::ObjectLayer kNonMoving = 0;
        static constexpr JPH::ObjectLayer kMoving = 1;
        static constexpr JPH::ObjectLayer kLayersCount = 2;
    }

    namespace broad_phase_layers
    {
        static constexpr auto kNonMoving   = JPH::BroadPhaseLayer(layers::kNonMoving);
        static constexpr auto kMoving      = JPH::BroadPhaseLayer(layers::kMoving);
        static constexpr auto kLayersCount = static_cast<uint32_t>(layers::kLayersCount);
    }
}
