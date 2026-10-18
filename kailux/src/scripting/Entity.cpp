#include "Entity.h"

#include "components/entt/LocalTransform.h"
#include "components/entt/PhysicsControlComponent.h"
#include "components/entt/TagComponent.h"
#include "scene/Scene.h"

namespace kailux
{
    TransformRef::TransformRef(Scene &scene, entt::entity handle) : pScene(&scene), mHandle(handle)
    {
    }
    glm::vec3 TransformRef::Position() const
    {
        return Get().position;
    }
    void TransformRef::SetPosition(const glm::vec3 &position)
    {
        auto transform{Get()};
        transform.position = position;
        Set(transform);
    }
    glm::quat TransformRef::Rotation() const
    {
        return Get().rotation;
    }
    void TransformRef::SetRotation(const glm::quat &rotation)
    {
        auto transform{Get()};
        transform.rotation = glm::normalize(rotation);
        Set(transform);
    }
    glm::vec3 TransformRef::Scale() const
    {
        return Get().scale;
    }
    void TransformRef::SetScale(const glm::vec3 &scale)
    {
        auto transform{Get()};
        transform.scale = scale;
        Set(transform);
    }
    void TransformRef::Translate(const glm::vec3 &delta)
    {
        auto transform{Get()};
        transform.position += delta;
        Set(transform);
    }
    void TransformRef::Rotate(const glm::quat &delta)
    {
        auto transform{Get()};
        transform.rotation = glm::normalize(delta * transform.rotation);
        Set(transform);
    }
    glm::vec3 TransformRef::Forward() const
    {
        return Get().rotation * glm::vec3{0.f, 0.f, -1.f};
    }
    glm::vec3 TransformRef::Right() const
    {
        return Get().rotation * glm::vec3{1.f, 0.f, 0.f};
    }
    glm::vec3 TransformRef::Up() const
    {
        return Get().rotation * glm::vec3{0.f, 1.f, 0.f};
    }
    Transform TransformRef::Get() const
    {
        if (const auto* local{pScene->GetEntityRegistry().try_get<LocalTransform>(mHandle)})
            return *local;
        return {};
    }
    void TransformRef::Set(const Transform &transform) const
    {
        pScene->SetLocalTransform(mHandle, transform);
    }

    RigidBody::RigidBody(Scene &scene, entt::entity handle) : pScene(&scene), mHandle(handle)
    {
    }

    bool RigidBody::Valid() const
    {
        return pScene->GetEntityRegistry().all_of<PhysicsComponent>(mHandle);
    }
    bool RigidBody::IsDynamic() const
    {
        const auto* physics{pScene->GetEntityRegistry().try_get<PhysicsComponent>(mHandle)};
        return physics && physics->IsDynamic();
    }
    void RigidBody::SetForce(const glm::vec3 &force)
    {
        auto& control{pScene->GetEntityRegistry().get_or_emplace<PhysicsControlComponent>(mHandle)};
        control.force      = force;
        control.applyForce = true;
    }
    void RigidBody::ClearForce()
    {
        if (auto* control{pScene->GetEntityRegistry().try_get<PhysicsControlComponent>(mHandle)})
        {
            control->force      = glm::vec3{0.f};
            control->applyForce = false;
        }
    }
    void RigidBody::AddImpulse(const glm::vec3 &impulse)
    {
        auto& control{pScene->GetEntityRegistry().get_or_emplace<PhysicsControlComponent>(mHandle)};
        control.impulse      = control.applyImpulse ? control.impulse + impulse : impulse;
        control.applyImpulse = true;
    }
    glm::vec3 RigidBody::GetVelocity() const
    {
        if (const auto* control{pScene->GetEntityRegistry().try_get<PhysicsControlComponent>(mHandle)})
            return control->velocity;
        return glm::vec3{0.f};
    }

    Entity::Entity() = default;

    Entity::Entity(Scene &scene, entt::entity handle) : pScene(&scene), mHandle(handle)
    {
    }

    std::string_view Entity::GetName() const
    {
        if (const auto* tag{TryGet<TagComponent>()})
            return tag->name;
        return {};
    }

    bool Entity::Valid() const
    {
        return pScene != nullptr && pScene->GetEntityRegistry().valid(mHandle);
    }

    Entity::operator bool() const
    {
        return Valid();
    }

    TransformRef Entity::GetTransform() const
    {
        return TransformRef{*pScene, mHandle};
    }

    RigidBody Entity::GetBody() const
    {
        return RigidBody{*pScene, mHandle};
    }

    entt::entity Entity::GetHandle() const
    {
        return mHandle;
    }

    bool Entity::operator==(const Entity &) const = default;

    entt::registry & Entity::GetRegistry() const
    {
        return pScene->GetEntityRegistry();
    }
}
