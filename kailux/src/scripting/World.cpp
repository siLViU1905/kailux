#include "World.h"

#include "components/entt/TagComponent.h"
#include "scene/Scene.h"

namespace kailux
{
    Entity World::Find(std::string_view name) const
    {
        const auto view{pScene->GetEntityRegistry().view<TagComponent>()};
        for (const auto entity : view)
            if (view.get<TagComponent>(entity).name == name)
                return {*pScene, entity};
        return {};
    }

    Entity World::Spawn(std::string_view name, const Transform &transform)
    {
        const auto entity{pScene->CreateEntity(name)};
        pScene->SetLocalTransform(entity, transform);
        return {*pScene, entity};
    }

    void World::Destroy(Entity entity)
    {
        if (entity.Valid())
            mPendingDestroy.push_back(entity.GetHandle());
    }

    void World::Bind(Scene &scene)
    {
        pScene = &scene;
        mPendingDestroy.clear();
    }

    void World::FlushDestroyed()
    {
        for (const auto entity : mPendingDestroy)
            if (pScene->GetEntityRegistry().valid(entity))
                pScene->DestroyEntity(entity);
        mPendingDestroy.clear();
    }

    void World::Clear()
    {
        pScene = nullptr;
        mPendingDestroy.clear();
    }
}
