#pragma once
#include "Entity.h"

namespace kailux
{
    class World
    {
    public:
        Entity Find(std::string_view name) const;
        Entity Spawn(std::string_view name, const Transform& transform = {});
        void   Destroy(Entity entity);

    private:
        void Bind(Scene& scene);
        void FlushDestroyed();
        void Clear();

        Scene*                    pScene{};
        std::vector<entt::entity> mPendingDestroy;

        friend class ScriptSystem;
    };
}