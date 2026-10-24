#include "World.h"

#include "ScriptContext.h"
#include "components/entt/CameraComponent.h"
#include "components/entt/PhysicsComponent.h"
#include "scene/Scene.h"

namespace kailux
{
    namespace details
    {
        static PhysicsCreationOptions to_creation_options(const BodySettings &settings)
        {
            PhysicsCreationOptions options;
            switch (settings.type)
            {
                case BodyType::Kinematic:
                    options.bodyType = PhysicsBodyType::Kinematic;
                    break;
                case BodyType::Dynamic:
                    options.bodyType = PhysicsBodyType::Dynamic;
                    break;
                default:
                    options.bodyType = PhysicsBodyType::Static;
                    break;
            }

            options.canBecomeDynamic = settings.type != BodyType::Static;
            options.mass             = settings.mass;
            options.isSensor         = settings.sensor;
            return options;
        }
    }

    Entity World::Find(std::string_view name) const
    {
        if (!pScene)
            return {};

        const auto entity{pScene->FindEntity(name)};
        return entity == entt::null ? Entity{} : Entity{*pScene, entity};
    }

    Entity World::Spawn(std::string_view name, const Transform &transform)
    {
        if (!pScene)
            return {};

        const auto entity{pScene->CreateEntity(name)};
        pScene->SetLocalTransform(entity, transform);
        return {*pScene, entity};
    }

    Entity World::SpawnPrimitive(std::string_view name, Primitive shape, const Transform &transform,
        std::optional<BodySettings> body)
    {
        if (!pScene || !pServices)
            return {};

        const auto type{shape == Primitive::Cube ? MeshType::Cube : MeshType::Sphere};
        const auto entity{pServices->SpawnPrimitive(*pScene, name, type, transform)};
        if (entity == entt::null)
            return {};

        Entity spawned{*pScene, entity};
        if (body)
            AddBody(spawned, *body);
        return spawned;
    }

    Entity World::SpawnMesh(const std::filesystem::path &path, std::string_view name, const Transform &transform,
        std::optional<BodySettings> body)
    {
        if (!pScene || !pServices)
            return {};

        std::optional<PhysicsCreationOptions> options;
        if (body)
            options = details::to_creation_options(*body);
        
        const auto entity{pServices->SpawnMesh(*pScene, path, name, transform, options)};
        return entity == entt::null ? Entity{} : Entity{*pScene, entity};
    }

    bool World::AddBody(Entity entity, const BodySettings &settings)
    {
        if (!pScene || !pServices || !entity.Valid())
            return false;

        return pServices->AddBody(*pScene, entity.GetHandle(), details::to_creation_options(settings));
    }

    void World::Destroy(Entity entity)
    {
        if (entity.Valid())
            mPendingDestroy.push_back(entity.GetHandle());
    }

    CameraRef World::GetMainCamera() const
    {
        if (!pScene)
            return {};

        const auto camera{pScene->GetPrimaryCamera()};
        if (camera == entt::null || !pScene->GetEntityRegistry().all_of<CameraComponent>(camera))
            return {};

        return {*pScene, camera, mViewportSize};
    }

    std::optional<RaycastHit> World::Raycast(const glm::vec3 &origin, const glm::vec3 &direction, float maxDistance,
                                             Entity ignore) const
    {
        if (!pScene)
            return std::nullopt;

        auto& registry{pScene->GetEntityRegistry()};
        auto* const* bodies{registry.ctx().find<PhysicsRegistry*>()};
        if (!bodies || !*bodies)
            return std::nullopt;

        BodyHandle ignored{};
        if (const auto* physics{ignore.TryGet<PhysicsComponent>()})
            ignored = physics->handle;

        const auto result{(*bodies)->CastRay(origin, direction, maxDistance, ignored)};
        if (!result)
            return std::nullopt;

        const auto entity{from_body_user_data(result->userData)};
        return RaycastHit{
            registry.valid(entity) ? Entity{*pScene, entity} : Entity{},
            result->point,
            result->normal,
            result->distance
        };
    }

    bool World::LoadScene(const std::filesystem::path &path)
    {
        if (!pServices)
            return false;
        return pServices->LoadScene(path);
    }

    void World::SetBuiltinCameraControl(bool enabled)
    {
        if (pServices)
            pServices->SetBuiltinCameraControl(enabled);
    }

    bool World::PlayOneShot(const std::filesystem::path &path, float gain)
    {
        if (!pServices)
            return false;
        return pServices->PlayOneShot(path, std::nullopt, gain);
    }

    bool World::PlayOneShotAt(const std::filesystem::path &path, const glm::vec3 &position, float gain)
    {
        if (!pServices)
            return false;
        return pServices->PlayOneShot(path, position, gain);
    }

    void World::SetMasterVolume(float volume)
    {
        if (pServices)
            pServices->SetMasterVolume(volume);
    }

    float World::GetMasterVolume() const
    {
        if (!pServices)
           return 0.f;
        return pServices->GetMasterVolume();
    }

    void World::Bind(Scene &scene, ScriptServices& services)
    {
        pScene = std::addressof(scene);
        pServices = std::addressof(services);
        mPendingDestroy.clear();
    }

    void World::SetViewportSize(glm::ivec2 size)
    {
        mViewportSize = size;
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
        pServices = nullptr;
        mPendingDestroy.clear();
    }
}
