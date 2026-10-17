#include "ScriptSystem.h"

#include "components/entt/NativeScriptComponent.h"

namespace kailux
{
    ScriptSystem::ScriptSystem() = default;

    void ScriptSystem::SetSimulationState(Scene& scene, SimulationState state)
    {
        if (state == mSimulationState)
            return;

        mSimulationState = state;
        if (state == SimulationState::Running)
            on_simulation_start(scene);
        else
            on_simulation_stop(scene);
    }

    SimulationState ScriptSystem::GetSimulationState() const
    {
        return mSimulationState;
    }

    void ScriptSystem::Update(Scene& scene, float deltaTime) const
    {
        if (mSimulationState != SimulationState::Running)
            return;

        const auto view{scene.GetEntityRegistry().view<NativeScriptComponent>()};
        view.each([deltaTime](auto& script)
        {
            if (script.instance)
                script.instance->OnUpdate(deltaTime);
        });
    }

    void ScriptSystem::connect(Scene& scene)
    {
        auto sink{scene.GetEntityRegistry().on_destroy<NativeScriptComponent>()};
        sink.disconnect<&ScriptSystem::on_component_destroyed>();
        sink.connect<&ScriptSystem::on_component_destroyed>();
    }

    void ScriptSystem::on_simulation_start(Scene& scene)
    {
        auto& registry{scene.GetEntityRegistry()};
        for (const auto entity : registry.view<NativeScriptComponent>())
        {
            auto& script{registry.get<NativeScriptComponent>(entity)};
            if (script.instance)
                continue;

            script.instance = script.Instantiate(scene, entity);
            script.instance->OnCreate();
        }
    }

    void ScriptSystem::on_simulation_stop(Scene& scene)
    {
        auto& registry{scene.GetEntityRegistry()};
        for (const auto entity : registry.view<NativeScriptComponent>())
        {
            auto& script{registry.get<NativeScriptComponent>(entity)};
            if (!script.instance)
                continue;

            script.instance->OnDestroy();
            script.instance.reset();
        }
    }

    void ScriptSystem::on_component_destroyed(entt::registry &registry, entt::entity entity)
    {
        auto& script{registry.get<NativeScriptComponent>(entity)};
        if (script.instance)
            script.instance->OnDestroy();
    }
}
