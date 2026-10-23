#include "Entity.h"

#include "Log.h"
#include "components/entt/LocalTransform.h"
#include "components/entt/NativeScriptComponent.h"
#include "components/entt/PhysicsControlComponent.h"
#include "components/entt/PhysicsInterpolation.h"
#include "components/entt/TagComponent.h"
#include "components/entt/WorldTransform.h"
#include "scene/Scene.h"

namespace kailux
{
     namespace details
    {
        static PhysicsRegistry* physics_registry(Scene &scene)
        {
            auto* const* registry{scene.GetEntityRegistry().ctx().find<PhysicsRegistry*>()};
            return registry ? *registry : nullptr;
        }

        static PhysicsBodyType to_physics_type(BodyType type)
        {
            switch (type)
            {
                case BodyType::Kinematic:
                    return PhysicsBodyType::Kinematic;
                case BodyType::Dynamic:
                    return PhysicsBodyType::Dynamic;
                default:
                    return PhysicsBodyType::Static;
            }
        }

        static BodyType from_physics_type(PhysicsBodyType type)
        {
            switch (type)
            {
                case PhysicsBodyType::Kinematic:
                    return BodyType::Kinematic;
                case PhysicsBodyType::Dynamic:
                    return BodyType::Dynamic;
                default:
                    return BodyType::Static;
            }
        }

         static void sync_body(Scene &scene, entt::entity entity, bool includeKinematic)
        {
            auto& registry{scene.GetEntityRegistry()};
            const auto* physics{registry.try_get<PhysicsComponent>(entity)};
            if (!physics || !physics->handle.Valid())
                return;
            if (physics->type == PhysicsBodyType::Kinematic && !includeKinematic)
                return;
            auto* bodies{physics_registry(scene)};
            if (!bodies)
                return;

            const auto world{Transform::from_matrix(registry.get<WorldTransform>(entity).model)};
            bodies->SetBodyTransform(physics->handle, world.position, world.rotation);
            if (auto* interpolation{registry.try_get<PhysicsInterpolation>(entity)})
                *interpolation = {world.position, world.position, world.rotation, world.rotation};
        }
    }

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
         details::sync_body(*pScene, mHandle, false);
    }

    glm::vec3 MaterialRef::Albedo() const
    {
         return {Get().albedoAndRoughness};
    }

    void MaterialRef::SetAlbedo(const glm::vec3 &albedo)
    {
         auto material{Get()};
         material.albedoAndRoughness.x = albedo.x;
         material.albedoAndRoughness.y = albedo.y;
         material.albedoAndRoughness.z = albedo.z;
         Set(material);
    }

    float MaterialRef::Roughness() const
    {
         return Get().albedoAndRoughness.w;
    }

    void MaterialRef::SetRoughness(float roughness)
    {
         auto material{Get()};
         material.albedoAndRoughness.w = roughness;
         Set(material);
    }

    float MaterialRef::Metallic() const
    {
         return Get().pbrParams.x;
    }

    void MaterialRef::SetMetallic(float metallic)
    {
         auto material{Get()};
         material.pbrParams.x = metallic;
         Set(material);
    }

    float MaterialRef::AO() const
    {
         return Get().pbrParams.y;
    }

    void MaterialRef::SetAO(float ao)
    {
         auto material{Get()};
         material.pbrParams.y = ao;
         Set(material);
    }

    MaterialRef::MaterialRef(Scene &scene, entt::entity handle) : pScene(&scene), mHandle(handle)
    {
    }

    RigidBody::RigidBody(Scene &scene, entt::entity handle) : pScene(&scene), mHandle(handle)
    {
    }

    Script * Entity::GetScriptInstance() const
    {
         const auto* script{TryGet<NativeScriptComponent>()};
         return script ? script->instance.get() : nullptr;
    }

    void Entity::AddScriptFactory(Scoped<Script>(*factory)()) const
    {
         if (!Valid() || Has<NativeScriptComponent>())
         {
             log::console.Warning("Cannot add script to '{}': invalid entity or it already has a script", GetName());
             return;
         }

         GetRegistry().emplace<NativeScriptComponent>(mHandle).Instantiate = factory;
    }

    MeshMaterialData MaterialRef::Get() const
    {
         if (const auto* material{pScene->GetEntityRegistry().try_get<MeshMaterialData>(mHandle)})
             return *material;
         return {};
    }

    void MaterialRef::Set(const MeshMaterialData &material) const
    {
         pScene->GetEntityRegistry().emplace_or_replace<MeshMaterialData>(mHandle, material);
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

    bool RigidBody::IsSensor() const
    {
         const auto* physics{pScene->GetEntityRegistry().try_get<PhysicsComponent>(mHandle)};
         return physics && physics->isSensor;
    }

    bool RigidBody::SetType(BodyType type)
    {
         auto* physics{pScene->GetEntityRegistry().try_get<PhysicsComponent>(mHandle)};
         auto* bodies{details::physics_registry(*pScene)};
         if (!physics || !physics->handle.Valid() || !bodies)
             return false;

         const auto physicsType{details::to_physics_type(type)};
         if (!bodies->SetBodyType(physics->handle, physicsType))
         {
             log::console.Warning("'{}' cannot change body type, it was not created with canBecomeDynamic",
                                  Entity{*pScene, mHandle}.GetName());
             return false;
         }
         physics->type = physicsType;
         details::sync_body(*pScene, mHandle, true);
         return true;
    }

    BodyType RigidBody::GetType() const
    {
         const auto* physics{pScene->GetEntityRegistry().try_get<PhysicsComponent>(mHandle)};
         return physics ? details::from_physics_type(physics->type) : BodyType::Static;
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

    void RigidBody::SetVelocity(const glm::vec3 &velocity)
    {
         auto& registry{pScene->GetEntityRegistry()};
         const auto* physics{registry.try_get<PhysicsComponent>(mHandle)};
         auto* bodies{details::physics_registry(*pScene)};
         if (!physics || !physics->handle.Valid() || !bodies)
             return;

         bodies->SetLinearVelocity(physics->handle, velocity);
         registry.get_or_emplace<PhysicsControlComponent>(mHandle).velocity = velocity;
    }

    glm::vec3 RigidBody::GetVelocity() const
    {
        if (const auto* control{pScene->GetEntityRegistry().try_get<PhysicsControlComponent>(mHandle)})
            return control->velocity;
        return glm::vec3{0.f};
    }

    void RigidBody::SetAngularVelocity(const glm::vec3 &velocity)
    {
         const auto* physics{pScene->GetEntityRegistry().try_get<PhysicsComponent>(mHandle)};
         auto* bodies{details::physics_registry(*pScene)};
         if (!physics || !physics->handle.Valid() || !bodies)
             return;

         bodies->SetAngularVelocity(physics->handle, velocity);
    }

    glm::vec3 RigidBody::GetAngularVelocity() const
    {
         const auto* physics{pScene->GetEntityRegistry().try_get<PhysicsComponent>(mHandle)};
         const auto* bodies{details::physics_registry(*pScene)};
         if (!physics || !physics->handle.Valid() || !bodies)
             return glm::vec3{0.f};

         return bodies->GetAngularVelocity(physics->handle);
    }

    float RigidBody::GetMass() const
    {
         const auto* physics{pScene->GetEntityRegistry().try_get<PhysicsComponent>(mHandle)};
         const auto* bodies{details::physics_registry(*pScene)};
         if (!physics || !physics->handle.Valid() || !bodies)
             return 0.f;

         return bodies->GetBodyMass(physics->handle);
    }

    void RigidBody::SetMass(float mass)
    {
         auto* physics{pScene->GetEntityRegistry().try_get<PhysicsComponent>(mHandle)};
         auto* bodies{details::physics_registry(*pScene)};
         if (!physics || !physics->handle.Valid() || !bodies)
             return;

         bodies->SetBodyMass(physics->handle, mass);
         physics->mass = std::max(mass, 0.f);
    }

    void RigidBody::Teleport(const glm::vec3 &position, const glm::quat &rotation)
    {
         auto& registry{pScene->GetEntityRegistry()};
         if (!registry.all_of<WorldTransform>(mHandle))
             return;

         const auto world{Transform::from_matrix(registry.get<WorldTransform>(mHandle).model)};
         const Transform target{position, glm::normalize(rotation), world.scale};
         pScene->SetWorldTransform(mHandle, target.GetModelMatrix());
         details::sync_body(*pScene, mHandle, true);
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

    MaterialRef Entity::GetMaterial() const
    {
        return MaterialRef{*pScene, mHandle};
    }

    RigidBody Entity::GetBody() const
    {
        return RigidBody{*pScene, mHandle};
    }

    AudioSource Entity::GetAudio() const
    {
        return pScene ? AudioSource{*pScene, mHandle} : AudioSource{};
    }

    AudioSource Entity::AddAudioSource(const std::filesystem::path &clip, const AudioSourceSettings &settings) const
    {
        if (!Valid())
            return {};

        auto& component{GetRegistry().get_or_emplace<AudioSourceComponent>(mHandle)};
        component.clip = clip.generic_string();
        component.settings = settings;
        component.loadFailed = false;

        AudioSource source{*pScene, mHandle};
        if (const auto* audio{source.GetRegistry()}; audio && audio->IsSourceValid(component.source))
            source.Prepare();
        return source;
    }

    bool Entity::AddScript(std::string_view name) const
    {
        if (!Valid() || Has<NativeScriptComponent>())
        {
            log::console.Warning("Cannot add script '{}' to '{}':"
                                 "invalid entity or it already has a script",
                                 name, GetName());
            return false;
        }

        if (!GetRegistry().emplace<NativeScriptComponent>(mHandle).Bind(name))
        {
            log::console.Warning("Script '{}' is not registered", name);
            return false;
        }
        return true;
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
