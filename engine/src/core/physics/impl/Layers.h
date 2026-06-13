#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>


namespace kailux
{
    namespace layers
    {
        static constexpr JPH::ObjectLayer s_NonMoving = 0;
        static constexpr JPH::ObjectLayer s_Moving = 1;
        static constexpr JPH::ObjectLayer s_LayersCount = 2;
    }

    namespace broad_phase_layers
    {
        static constexpr auto s_NonMoving   = JPH::BroadPhaseLayer(layers::s_NonMoving);
        static constexpr auto s_Moving      = JPH::BroadPhaseLayer(layers::s_Moving);
        static constexpr auto s_LayersCount = static_cast<uint32_t>(layers::s_LayersCount);
    }
}
