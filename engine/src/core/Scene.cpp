#include "Scene.h"

#include "components/entt/CameraComponent.h"
#include "components/entt/MeshComponent.h"
#include "components/entt/MaterialComponent.h"
#include "components/entt/TagComponent.h"
#include "components/gpu/CameraData.h"

namespace kailux
{
    Scene::Scene() : m_MainCameraEntity(entt::null),
                     m_Sun(entt::null),
                     m_Ambient(SceneData().ambient),
                     m_MeshEntityNameCount(0)
    {
        m_Sun = createSunEntity({});
    }

    Scene::Scene(Scene &&other) noexcept : m_EntityRegistry(std::move(other.m_EntityRegistry)),
                                           m_MainCameraEntity(other.m_MainCameraEntity),
                                           m_Sun(other.m_Sun),
                                           m_Ambient(other.m_Ambient),
                                           m_MeshEntityNameCount(other.m_MeshEntityNameCount)
    {
    }

    Scene &Scene::operator=(Scene &&other) noexcept
    {
        if (this != &other)
        {
            m_EntityRegistry = std::move(other.m_EntityRegistry);
            m_MainCameraEntity = other.m_MainCameraEntity;
            m_Sun = other.m_Sun;
            m_Ambient = other.m_Ambient;
            m_MeshEntityNameCount = other.m_MeshEntityNameCount;
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
            glm::vec4(camera.getPosition(), CameraData::s_DefaultExposure)
        );
        return entity;
    }

    entt::entity Scene::createMeshEntity(
        std::string_view name,
        MeshHandle handle,
        TextureSet textureSet,
        const MeshTransformData &transform,
        const MeshMaterialData &material
    )
    {
        auto entity = createEntity(name);
        m_EntityRegistry.emplace<MeshComponent>(
            entity,
            handle
        );
        m_EntityRegistry.emplace<MaterialComponent>(
            entity,
            textureSet
            );
        m_EntityRegistry.emplace<MeshTransformData>(
            entity,
            transform
        );
        m_EntityRegistry.emplace<MeshMaterialData>
        (
            entity,
            material
        );
        return entity;
    }

    entt::registry &Scene::getEntityRegistry()
    {
        return m_EntityRegistry;
    }

    const entt::registry &Scene::getEntityRegistry() const
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

    entt::entity Scene::getSun() const
    {
        return m_Sun;
    }

    SceneData Scene::getData() const
    {
        const auto &sunData = m_EntityRegistry.get<SunData>(m_Sun);
        return {sunData, m_Ambient};
    }

    glm::vec4 &Scene::getAmbient()
    {
        return m_Ambient;
    }

    const glm::vec4 &Scene::getAmbient() const
    {
        return m_Ambient;
    }

    std::string Scene::getMeshEntityName()
    {
        return "Mesh" + std::to_string(m_MeshEntityNameCount++);
    }

    entt::entity Scene::createEntity(std::string_view name)
    {
        entt::entity entity = m_EntityRegistry.create();
        m_EntityRegistry.emplace<TagComponent>(entity, name.data());
        return entity;
    }

    entt::entity Scene::createSunEntity(const SunData &data)
    {
        auto entity = createEntity(s_SunName);
        m_EntityRegistry.emplace<SunData>(
            entity,
            data
        );
        return entity;
    }
}
