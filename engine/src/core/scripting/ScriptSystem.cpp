#include "ScriptSystem.h"

#include "core/components/entt/NativeScriptComponent.h"

namespace kailux
{
    ScriptSystem::ScriptSystem(Scene &scene) : mScene(scene)
    {
    }

    void ScriptSystem::SetSimulationState(SimulationState state)
    {
        if (state == mSimulationState)
            return;

        mSimulationState = state;
        if (state == SimulationState::Running)
            OnSimulationStart();
        else
            OnSimulationStop();
    }

    SimulationState ScriptSystem::GetSimulationState() const
    {
        return mSimulationState;
    }

    void ScriptSystem::Update(float deltaTime) const
    {
        if (mSimulationState != SimulationState::Running)
            return;

        const auto view{mScene.get().GetEntityRegistry().view<NativeScriptComponent>()};
        view.each([deltaTime](auto& script)
        {
            if (script.instance)
                script.instance->OnUpdate(deltaTime);
        });
    }

    void ScriptSystem::SetConnections() const
    {
        mScene.get().GetEntityRegistry()
                .on_destroy<NativeScriptComponent>()
                .connect<&ScriptSystem::on_component_destroyed>();
    }

    void ScriptSystem::OnSimulationStart() const
    {
        auto& registry{mScene.get().GetEntityRegistry()};
        for (const auto entity : registry.view<NativeScriptComponent>())
        {
            auto& script{registry.get<NativeScriptComponent>(entity)};
            if (script.instance)
                continue;

            script.instance = script.Instantiate(mScene.get(), entity);
            script.instance->OnCreate();
        }
    }

    void ScriptSystem::OnSimulationStop() const
    {
        auto& registry{mScene.get().GetEntityRegistry()};
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
