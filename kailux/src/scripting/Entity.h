#pragma once

#include <entt/entt.hpp>
#include "components/math/Transform.h"

namespace kailux
{
    class Scene;
    class Entity;
    class Script;

    enum class BodyType : uint8_t
    {
        Static,
        Kinematic,
        Dynamic
    };

    struct BodySettings
    {
        BodyType type{BodyType::Dynamic};
        float    mass{};
        bool     sensor{false};
    };

    enum class Primitive : uint8_t
    {
        Cube,
        Sphere
    };

    class TransformRef
    {
    public:
        glm::vec3 Position() const;
        void      SetPosition(const glm::vec3& position);
        glm::quat Rotation() const;
        void      SetRotation(const glm::quat& rotation);
        glm::vec3 Scale() const;
        void      SetScale(const glm::vec3& scale);

        void Translate(const glm::vec3& delta);
        void Rotate(const glm::quat& delta);

        glm::vec3 Forward() const;
        glm::vec3 Right() const;
        glm::vec3 Up() const;

    private:
        explicit TransformRef(Scene& scene, entt::entity handle);

        Transform Get() const;
        void      Set(const Transform& transform) const;

        Scene*       pScene{};
        entt::entity mHandle{entt::null};

        friend class Entity;
    };

    class RigidBody
    {
    public:
        bool Valid() const;

        bool IsDynamic() const;
        bool IsSensor() const;

        bool     SetType(BodyType type);
        BodyType GetType() const;

        void SetForce(const glm::vec3& force);
        void ClearForce();
        void AddImpulse(const glm::vec3& impulse);

        void      SetVelocity(const glm::vec3& velocity);
        glm::vec3 GetVelocity() const;
        void      SetAngularVelocity(const glm::vec3& velocity);
        glm::vec3 GetAngularVelocity() const;

        float GetMass() const;
        void  SetMass(float mass);

        void Teleport(const glm::vec3& position, const glm::quat& rotation);

    private:
        explicit RigidBody(Scene& scene, entt::entity handle);

        Scene*       pScene{};
        entt::entity mHandle{entt::null};

        friend class Entity;
    };

    class Entity
    {
    public:
        Entity();
        Entity(Scene& scene, entt::entity handle);

        template<typename Component>
        Component& Get() const
        {
            return GetRegistry().template get<Component>(mHandle);
        }

        template<typename Component>
        Component* TryGet() const
        {
            return Valid() ? GetRegistry().template try_get<Component>(mHandle) : nullptr;
        }

        template<typename... Components>
        bool Has() const
        {
            return Valid() && GetRegistry().template all_of<Components...>(mHandle);
        }

        template<typename Component, typename... Args>
        Component& Add(Args&&... args) const
        {
            return GetRegistry().template emplace_or_replace<Component>(mHandle, std::forward<Args>(args)...);
        }

        template<typename Component>
        void Remove() const
        {
            GetRegistry().template remove<Component>(mHandle);
        }

        std::string_view GetName() const;
        bool             Valid() const;
        explicit operator bool() const;

        TransformRef GetTransform() const;
        RigidBody    GetBody() const;

        template<typename T>
        T* GetScript() const
        {
            return dynamic_cast<T*>(GetScriptInstance());
        }

        bool AddScript(std::string_view name) const;

        template<typename T>
        void AddScript() const
        {
            AddScriptFactory([]() -> Scoped<Script>
            {
                return create_scoped<T>();
            });
        }

        entt::entity GetHandle() const;

        bool operator==(const Entity &) const;

    private:
        entt::registry& GetRegistry() const;

        Script* GetScriptInstance() const;
        void    AddScriptFactory(Scoped<Script>(*factory)()) const;

        Scene*       pScene{};
        entt::entity mHandle{entt::null};
    };
}