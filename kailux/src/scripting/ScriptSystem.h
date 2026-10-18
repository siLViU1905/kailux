#pragma once

#include "ScriptContext.h"
#include "scene/Scene.h"

namespace kailux
{
    struct NativeScriptComponent;

    class ScriptSystem
    {
    public:
        ScriptSystem();

        void            SetSimulationState(Scene& scene, SimulationState state);
        SimulationState GetSimulationState() const;

        void SetFixedStep(float step);

        void Update(Scene& scene, float deltaTime, const InputSource& input);

        static void connect(Scene& scene);

    private:
        void OnSimulationStart(Scene& scene);
        void OnSimulationStop(Scene& scene);

        void Instantiate(Scene& scene, entt::entity entity, NativeScriptComponent& script);

        static void on_component_destroyed(entt::registry& registry, entt::entity entity);

        SimulationState mSimulationState{SimulationState::Paused};

        Scoped<ScriptContext>     mContext;
        std::vector<entt::entity> mUpdateQueue;
    };
}