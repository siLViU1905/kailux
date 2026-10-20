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

    void ScriptSystem::SetServices(ScriptServices &&services)
    {
        mContext->services = std::move(services);
    }

    void ScriptSystem::BeginFrame(float deltaTime, const InputSource& input, glm::vec2 scrollDelta)
    {
        if (mSimulationState != SimulationState::Running)
            return;

        mContext->time.Advance(deltaTime);
        mContext->input.BeginFrame(input, scrollDelta);

       if (input.Valid())
           mContext->world.SetViewportSize(input.GetWindowSize());
    }

    void ScriptSystem::FixedUpdate(Scene &scene, float fixedStep)
    {
        if (mSimulationState != SimulationState::Running)
            return;

        auto& registry{scene.GetEntityRegistry()};
        const auto view{registry.view<NativeScriptComponent>()};
        mUpdateQueue.assign(view.begin(), view.end());

        for (const auto entity : mUpdateQueue)
            if (auto* script{registry.try_get<NativeScriptComponent>(entity)}; script && script->instance)
                script->instance->OnFixedUpdate(fixedStep);
    }

    void ScriptSystem::DispatchContacts(Scene &scene, std::span<const ContactEvent> events)
    {
        if (mSimulationState != SimulationState::Running)
            return;

        auto& registry{scene.GetEntityRegistry()};
        const auto notify{[&](entt::entity self, entt::entity other, const ContactEvent& event)
        {
            if (!registry.valid(self))
                return;
            auto* script{registry.try_get<NativeScriptComponent>(self)};
            if (!script || !script->instance)
                return;
            const Entity otherEntity{registry.valid(other) ? Entity{scene, other} : Entity{}};
            auto& instance{*script->instance};
            if (event.sensor)
                event.began ? instance.OnTriggerEnter(otherEntity) : instance.OnTriggerExit(otherEntity);
            else
                event.began ? instance.OnCollisionEnter(otherEntity) : instance.OnCollisionExit(otherEntity);
        }};

        for (const auto& event : events)
        {
            const auto first{from_body_user_data(event.first)};
            const auto second{from_body_user_data(event.second)};
            notify(first, second, event);
            notify(second, first, event);
        }
    }

    void ScriptSystem::Update(Scene &scene, float deltaTime)
    {
        if (mSimulationState != SimulationState::Running)
            return;

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
        InstantiatePending(scene);
    }

    void ScriptSystem::connect(Scene& scene)
    {
        auto sink{scene.GetEntityRegistry().on_destroy<NativeScriptComponent>()};
        sink.disconnect<&ScriptSystem::on_component_destroyed>();
        sink.connect<&ScriptSystem::on_component_destroyed>();
    }

    void ScriptSystem::OnSimulationStart(Scene& scene)
    {
        mContext->world.Bind(scene, mContext->services);
        mContext->time.Reset();
        mContext->input.Reset();

        InstantiatePending(scene);
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

        if (mContext->services.setBuiltinCameraControl)
            mContext->services.setBuiltinCameraControl(true);

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

    void ScriptSystem::InstantiatePending(Scene &scene)
    {
        auto& registry{scene.GetEntityRegistry()};
        const auto view{registry.view<NativeScriptComponent>()};
        mUpdateQueue.clear();
        for (const auto entity : view)
        {
            const auto& script{view.get<NativeScriptComponent>(entity)};
            if (!script.instance && script.Instantiate)
                mUpdateQueue.push_back(entity);
        }

        for (const auto entity : mUpdateQueue)
            if (auto* script{registry.try_get<NativeScriptComponent>(entity)}; script && !script->instance)
                Instantiate(scene, entity, *script);
    }

    void ScriptSystem::on_component_destroyed(entt::registry &registry, entt::entity entity)
    {
        auto& script{registry.get<NativeScriptComponent>(entity)};
        if (script.instance)
            script.instance->OnDestroy();
    }
}
