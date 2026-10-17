#pragma once
#include "scene/Scene.h"

namespace kailux
{
    class ScriptSystem
    {
    public:
        ScriptSystem();

        void            SetSimulationState(Scene& scene, SimulationState state);
        SimulationState GetSimulationState() const;

        void Update(Scene& scene, float deltaTime) const;

        static void connect(Scene& scene);

    private:
        static void on_simulation_start(Scene& scene);
        static void on_simulation_stop(Scene& scene);

        static void on_component_destroyed(entt::registry& registry, entt::entity entity);

        SimulationState mSimulationState{SimulationState::Paused};
    };
}