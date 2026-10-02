#pragma once
#include "core/Core.h"

namespace kailux
{
    class Scene;

    class ScriptableEntity
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ScriptableEntity)

        ScriptableEntity(Scene& scene, entt::entity handle);

        template<typename Component>
        decltype(auto) GetComponent(this auto&& self)
        {
            return self.pScene->GetEntityRegistry().template get<Component>(self.mHandle);
        }

        bool Valid() const;

        virtual ~ScriptableEntity();

    protected:
        virtual void OnCreate() = 0;
        virtual void OnUpdate(float deltaTime) = 0;
        virtual void OnDestroy() = 0;

        entt::entity       GetHandle() const;
        const Scene&       GetScene() const;

    private:
        Scene*       pScene{};
        entt::entity mHandle{entt::null};

        friend class ScriptSystem;
    };
}
