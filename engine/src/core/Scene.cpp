#include "Scene.h"

#include "components/entt/CameraComponent.h"
#include "components/entt/MeshComponent.h"
#include "components/entt/TagComponent.h"
#include "components/gpu/CameraData.h"

namespace kailux
{
    Scene::Scene() : m_MainCameraEntity()
    {
    }

    Scene::Scene(Scene &&other) noexcept : m_EntityRegistry(std::move(other.m_EntityRegistry)),
                                           m_MainCameraEntity(other.m_MainCameraEntity)
    {
    }

    Scene &Scene::operator=(Scene &&other) noexcept
    {
        if (this != &other)
        {
            m_EntityRegistry = std::move(other.m_EntityRegistry);
            m_MainCameraEntity = other.m_MainCameraEntity;
        }
        return *this;
    }

    Scene Scene::create()
    {
        return {};
    }

    entt::entity Scene::createCameraEntity(std::string_view name, const Camera &camera, bool isPrimary)
    {
        auto entity = createEntity(name);
        m_EntityRegistry.emplace<CameraComponent>(
            entity,
            camera,
            isPrimary
        );
        m_EntityRegistry.emplace<CameraData>(
            entity,
            camera.getProjection(),
            camera.getView(),
            glm::vec4(camera.getPosition(), 1.f)
        );
        return entity;
    }

    entt::entity Scene::createMeshEntity(std::string_view name, MeshHandle handle, const MeshTransformData &transform)
    {
        auto entity = createEntity(name);
        m_EntityRegistry.emplace<MeshComponent>(
            entity,
            handle
        );
        m_EntityRegistry.emplace<MeshTransformData>(
            entity,
            transform
        );
        return entity;
    }

    entt::registry & Scene::getEntityRegistry()
    {
        return m_EntityRegistry;
    }

    const entt::registry & Scene::getEntityRegistry() const
    {
        return m_EntityRegistry;
    }

    entt::entity Scene::getMainCamera() const
    {
        return m_MainCameraEntity;
    }

    void Scene::setMainCamera(entt::entity camera)
    {
        m_MainCameraEntity = camera;
    }

    entt::entity Scene::createEntity(std::string_view name)
    {
        entt::entity entity = m_EntityRegistry.create();
        m_EntityRegistry.emplace<TagComponent>(entity, name.data());
        return entity;
    }
}
