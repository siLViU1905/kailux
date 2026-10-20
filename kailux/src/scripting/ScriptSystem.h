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
        void SetServices(ScriptServices&& services);

        void BeginFrame(float deltaTime, const InputSource& input, glm::vec2 scrollDelta);
        void FixedUpdate(Scene& scene, float fixedStep);
        void DispatchContacts(Scene& scene, std::span<const ContactEvent> events);
        void Update(Scene& scene, float deltaTime);

        static void connect(Scene& scene);

    private:
        void OnSimulationStart(Scene& scene);
        void OnSimulationStop(Scene& scene);

        void Instantiate(Scene& scene, entt::entity entity, NativeScriptComponent& script);
        void InstantiatePending(Scene& scene);

        static void on_component_destroyed(entt::registry& registry, entt::entity entity);

        SimulationState mSimulationState{SimulationState::Paused};

        Scoped<ScriptContext>     mContext;
        std::vector<entt::entity> mUpdateQueue;
    };
}