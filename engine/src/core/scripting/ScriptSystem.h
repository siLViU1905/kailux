#pragma once
#include "core/scene/Scene.h"

namespace kailux
{
    class ScriptSystem
    {
    public:
        ScriptSystem(Scene& scene);

        void            SetSimulationState(SimulationState state);
        SimulationState GetSimulationState() const;

        void Update(float deltaTime) const;

        void SetConnections() const;

    private:
        void OnSimulationStart() const;
        void OnSimulationStop() const;

        static void on_component_destroyed(entt::registry& registry, entt::entity entity);

        std::reference_wrapper<Scene> mScene;
        SimulationState               mSimulationState{SimulationState::Paused};
    };
}