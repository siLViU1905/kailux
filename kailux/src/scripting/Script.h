#pragma once

#include "Core.h"
#include "Entity.h"

namespace kailux
{
    class World;
    class Input;
    class Time;
    struct ScriptContext;
    class ScriptParams;

    class Script
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Script)

        virtual ~Script();

    protected:
        virtual void OnCreate();
        virtual void OnUpdate(float deltaTime);
        virtual void OnFixedUpdate(float fixedStep);
        virtual void OnDestroy();

        Entity              GetSelf() const;
        World&              GetWorld() const;
        Input&              GetInput() const;
        const Time&         GetTime() const;
        const ScriptParams& GetParams() const;

        virtual void OnCollisionEnter(Entity other);
        virtual void OnCollisionExit(Entity other);

        virtual void OnTriggerEnter(Entity other);
        virtual void OnTriggerExit(Entity other);

        template<typename Component>
        Component& Get() const
        {
            return mSelf.Get<Component>();
        }
        template<typename Component>
        Component* TryGet() const
        {
            return mSelf.TryGet<Component>();
        }
        template<typename... Components>
        bool Has() const
        {
            return mSelf.Has<Components...>();
        }

    private:
        void Attach(ScriptContext& context, Entity self);

        ScriptContext* pContext{};
        Entity         mSelf;

        friend class ScriptSystem;
    };
}
