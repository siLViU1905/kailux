#pragma once
#include "core/Core.h"
#include "core/scripting/ScriptableEntity.h"

namespace kailux
{
    class Scene;

    struct NativeScriptComponent
    {
        Scoped<ScriptableEntity> instance;

        using OnInstantiate = Scoped<ScriptableEntity>(*)(Scene&, entt::entity);
        OnInstantiate Instantiate{};

        template<typename Script>
        void Bind()
        {
            Instantiate = [](Scene& scene, entt::entity handle) -> Scoped<ScriptableEntity>
            {
                return create_scoped<Script>(scene, handle);
            };
        }
    };
}
