#include "ScriptSystem.h"

#include "components/entt/NativeScriptComponent.h"

namespace kailux
{
    ScriptSystem::ScriptSystem() : mContext(create_scoped<ScriptContext>())
    {
    }

    void ScriptSystem::SetSimulationState(Scene& scene, SimulationState state)
    {
        if (state == mSimulationState)
            return;

        mSimulationState = state;
        if (state == SimulationState::Running)
            OnSimulationStart(scene);
        else
            OnSimulationStop(scene);
    }

    SimulationState ScriptSystem::GetSimulationState() const
    {
        return mSimulationState;
    }

    void ScriptSystem::SetFixedStep(float step)
    {
        mContext->time.mFixedStep = step;
    }

    void ScriptSystem::Update(Scene& scene, float deltaTime, const InputSource& input)
    {
        if (mSimulationState != SimulationState::Running)
            return;

        mContext->time.Advance(deltaTime);
        mContext->input.BeginFrame(input);

        auto& registry{scene.GetEntityRegistry()};
        const auto view{registry.view<NativeScriptComponent>()};
        mUpdateQueue.assign(view.begin(), view.end());

        for (const auto entity: mUpdateQueue)
        {
            auto *script{registry.try_get<NativeScriptComponent>(entity)};
            if (!script)
                continue;
            if (!script->instance)
                Instantiate(scene, entity, *script);
            if (script->instance)
                script->instance->OnUpdate(deltaTime);
        }

        mContext->world.FlushDestroyed();
    }

    void ScriptSystem::connect(Scene& scene)
    {
        auto sink{scene.GetEntityRegistry().on_destroy<NativeScriptComponent>()};
        sink.disconnect<&ScriptSystem::on_component_destroyed>();
        sink.connect<&ScriptSystem::on_component_destroyed>();
    }

    void ScriptSystem::OnSimulationStart(Scene& scene)
    {
        mContext->world.Bind(scene);
        mContext->time.Reset();
        mContext->input.Reset();

        auto& registry{scene.GetEntityRegistry()};
        for (const auto entity : registry.view<NativeScriptComponent>())
        {
            auto& script{registry.get<NativeScriptComponent>(entity)};
            if (!script.instance)
                Instantiate(scene, entity, script);
        }
    }

    void ScriptSystem::OnSimulationStop(Scene& scene)
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

        mContext->world.Clear();
        mContext->input.Reset();
    }

    void ScriptSystem::Instantiate(Scene &scene, entt::entity entity, NativeScriptComponent &script)
    {
        if (!script.Instantiate)
            return;

        script.instance = script.Instantiate();
        script.instance->Attach(*mContext, {scene, entity});
        script.instance->OnCreate();
    }

    void ScriptSystem::on_component_destroyed(entt::registry &registry, entt::entity entity)
    {
        auto& script{registry.get<NativeScriptComponent>(entity)};
        if (script.instance)
            script.instance->OnDestroy();
    }
}
