#include "ScriptableEntity.h"

namespace kailux
{
    ScriptableEntity::ScriptableEntity() = default;

    ScriptableEntity::ScriptableEntity(Scene &scene, entt::entity handle) : pScene(&scene), mHandle(handle)
    {
    }

    ScriptableEntity::ScriptableEntity(ScriptableEntity &&other) noexcept : pScene(std::exchange(other.pScene, nullptr)),
                                                                            mHandle(std::exchange(other.mHandle, entt::null))
    {
    }

    ScriptableEntity & ScriptableEntity::operator=(ScriptableEntity &&other) noexcept
    {
        if (this != &other)
        {
            pScene = std::exchange(other.pScene, nullptr);
            mHandle = std::exchange(other.mHandle, entt::null);
        }
        return *this;
    }

    bool ScriptableEntity::Valid() const
    {
        return pScene != nullptr
               && mHandle != entt::null;
    }

    ScriptableEntity::~ScriptableEntity() = default;

    entt::entity ScriptableEntity::GetHandle() const
    {
        return mHandle;
    }

    const Scene & ScriptableEntity::GetScene() const
    {
        return *pScene;
    }
}
