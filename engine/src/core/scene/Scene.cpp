#include "Scene.h"

#include "../components/entt/CameraComponent.h"
#include "../components/entt/MeshComponent.h"
#include "../components/entt/MaterialComponent.h"
#include "../components/entt/TagComponent.h"
#include "../components/gpu/CameraData.h"
#include <nlohmann/json.hpp>

#include "../components/entt/HierarchyComponent.h"
#include "../components/entt/PhysicsComponent.h"
#include "../components/gpu/TransformComponent.h"
#include "core/components/entt/PhysicsControlComponent.h"

namespace kailux
{
    Scene::Scene() = default;

    Scene::Scene(Scene &&other) noexcept : mName(std::move(other.mName)),
                                           mSavePath(std::move(other.mSavePath)),
                                           mEntityRegistry(std::move(other.mEntityRegistry)),
                                           mMainCameraEntity(other.mMainCameraEntity),
                                           mSun(other.mSun),
                                           mMeshEntityNameCount(other.mMeshEntityNameCount),
                                           mLightEntityNameCount(other.mLightEntityNameCount)
    {
    }

    Scene &Scene::operator=(Scene &&other) noexcept
    {
        if (this != &other)
        {
            mName = std::move(other.mName);
            mSavePath = std::move(other.mSavePath);
            mEntityRegistry = std::move(other.mEntityRegistry);
            mMainCameraEntity = other.mMainCameraEntity;
            mSun = other.mSun;
            mMeshEntityNameCount = other.mMeshEntityNameCount;
            mLightEntityNameCount = other.mLightEntityNameCount;
        }
        return *this;
    }

    Scene Scene::create(std::string_view name)
    {
        Scene scene;
        scene.mName = name;
        scene.mSun = scene.createSunEntity({});
        return scene;
    }

    void Scene::update()
    {
        updateTransforms();
    }

    entt::entity Scene::createCameraEntity(std::string_view name, bool isPrimary, int width, int height)
    {
        auto entity = createEntity(name);
        attachCamera(entity, {isPrimary});
        return entity;
    }

    std::optional<entt::entity> Scene::createMeshEntity(
        std::string_view name,
        const MeshComponent &component,
        MaterialHandle materialHandle,
        const MeshTransformData &transform,
        const MeshMaterialData &material,
        entt::entity parent
    )
    {
        if (mEntityRegistry.view<MeshComponent>().size() >= details::kMaxMeshes)
            return std::nullopt;

        auto entity = createEntity(name);
        setLocalTransform(entity, transform);

        if (!attachMesh(entity, component, materialHandle, material))
        {
            destroyEntity(entity);
            return std::nullopt;
        }

        if (parent != entt::null)
            setParent(entity, parent);

        return entity;
    }

    entt::entity Scene::createParentEntity(std::string_view name)
    {
        return createEntity(name);
    }

    std::optional<entt::entity> Scene::createPointLightEntity(std::string_view name, const GizmoComponent &component, const glm::vec3 &position)
    {
        if (mEntityRegistry.view<PointLightData>().size() >= details::kMaxPointLights)
            return std::nullopt;
        auto entity = createEntity(name);

        attachPointLight(entity, component, {});

        MeshTransformData transform;
        transform.position = position;
        mEntityRegistry.emplace<TransformComponent>(
            entity,
            transform,
            glm::mat4(1.f),
            transform.getModelMatrix()
        );
        mEntityRegistry.emplace<HierarchyComponent>(entity);

        return entity;
    }

    entt::registry &Scene::getEntityRegistry()
    {
        return mEntityRegistry;
    }

    const entt::registry &Scene::getEntityRegistry() const
    {
        return mEntityRegistry;
    }

    entt::entity Scene::getMainCamera() const
    {
        return mMainCameraEntity;
    }

    void Scene::setMainCamera(entt::entity camera)
    {
        mMainCameraEntity = camera;
    }

    entt::entity Scene::getSun() const
    {
        return mSun;
    }

    SceneData Scene::getData() const
    {
        return {getLightData()};
    }

    LightsData Scene::getLightData() const
    {
        LightsData data;
        data.directional = mEntityRegistry.get<SunData>(mSun);
        auto view = mEntityRegistry.view<PointLightData, TransformComponent>();
        uint32_t index = 0;
        for (auto entity : view)
        {
            auto light = view.get<PointLightData>(entity);
            const auto& transform = view.get<TransformComponent>(entity);
            auto pos = glm::vec3(transform.worldMatrix[3]);
            light.positionAndIntensity = glm::vec4(pos, light.positionAndIntensity.w);
            data.pointLights[index++] = light;
        }
        data.pointLightCount = index;
        return data;
    }

    std::string_view Scene::getName() const
    {
        return mName;
    }

    std::string Scene::getMeshEntityName()
    {
        return std::format("Mesh{}", mMeshEntityNameCount++);
    }

    std::string Scene::getLightEntityName()
    {
        return std::format("Light{}", mLightEntityNameCount++);
    }

    void Scene::setSavePath(const std::filesystem::path &path)
    {
        mSavePath = path;
    }

    const std::filesystem::path & Scene::getSavePath() const
    {
        return mSavePath;
    }

    void Scene::setMeta(const SceneMeta &meta)
    {
        mName                 = meta.name;
        mSavePath             = meta.savePath;
        mMeshEntityNameCount  = meta.meshNameCount;
        mLightEntityNameCount = meta.lightNameCount;
    }

    SceneMeta Scene::getMeta() const
    {
        return {
            mName,
            mSavePath,
            mMeshEntityNameCount,
            mLightEntityNameCount
        };
    }

    bool Scene::attachMesh(entt::entity entity, const MeshComponent &component, MaterialHandle materialHandle,
                           const MeshMaterialData &material)
    {
        if (!mEntityRegistry.valid(entity))
            return false;
        if (!mEntityRegistry.all_of<MeshComponent>(entity) &&
            mEntityRegistry.view<MeshComponent>().size() >= details::kMaxMeshes)
            return false;

        mEntityRegistry.emplace_or_replace<MeshComponent>(entity, component);
        mEntityRegistry.emplace_or_replace<MaterialComponent>(entity, materialHandle);
        mEntityRegistry.emplace_or_replace<MeshMaterialData>(entity, material);
        return true;

    }

    bool Scene::attachMeshSource(entt::entity entity, const MeshSourceComponent &source)
    {
        if (mEntityRegistry.valid(entity))
        {
            mEntityRegistry.emplace_or_replace<MeshSourceComponent>(entity, source);
            return true;
        }
        return false;
    }

    bool Scene::attachPointLight(entt::entity entity, const GizmoComponent &component, const PointLightRecord &light)
    {
        if (!mEntityRegistry.valid(entity))
            return false;
        if (!mEntityRegistry.all_of<PointLightData>(entity) &&
            mEntityRegistry.view<PointLightData>().size() >= details::kMaxPointLights)
            return false;

        mEntityRegistry.emplace_or_replace<GizmoComponent>(entity, component);

        auto &data = mEntityRegistry.emplace_or_replace<PointLightData>(entity);
        data.positionAndIntensity.w = light.intensity;
        data.range.x                = light.range;
        data.colorAndEnabled        = light.colorAndEnabled;
        return true;
    }

    void Scene::attachPhysics(entt::entity entity, PhysicsComponent component)
    {
        if (mEntityRegistry.valid(entity))
        {
            mEntityRegistry.emplace_or_replace<PhysicsComponent>(entity, component);
            mEntityRegistry.emplace<PhysicsControlComponent>(entity);
        }
    }

    void Scene::attachCamera(entt::entity entity, const CameraComponent &component)
    {
        if (!mEntityRegistry.valid(entity))
            return;

        mEntityRegistry.emplace_or_replace<CameraComponent>(entity, component);
    }

    void Scene::setLocalTransform(entt::entity entity, const MeshTransformData &transform)
    {
        if (!mEntityRegistry.valid(entity))
            return;

        auto& component = mEntityRegistry.emplace_or_replace<TransformComponent>(entity);
        component.transform = transform;
        component.worldMatrix = transform.getModelMatrix();
    }

    void Scene::setParent(entt::entity child, entt::entity parent)
    {
        if (child == parent || !mEntityRegistry.valid(child))
            return;

        detachFromParent(child);
        if (parent == entt::null || !mEntityRegistry.valid(parent))
            return;

        mEntityRegistry.get_or_emplace<HierarchyComponent>(child).parent = parent;
        mEntityRegistry.get_or_emplace<HierarchyComponent>(parent).children.push_back(child);
    }
    void Scene::detachFromParent(entt::entity child)
    {
        auto *hierarchy = mEntityRegistry.try_get<HierarchyComponent>(child);
        if (!hierarchy || hierarchy->parent == entt::null)
            return;

        if (auto *old = mEntityRegistry.try_get<HierarchyComponent>(hierarchy->parent))
            std::erase(old->children, child);

        hierarchy->parent = entt::null;
    }

    void Scene::destroyEntity(entt::entity entity)
    {
        if (!mEntityRegistry.valid(entity))
            return;

        if (const auto *hierarchy = mEntityRegistry.try_get<HierarchyComponent>(entity))
        {
            auto children = hierarchy->children;
            for (auto child : children)
                destroyEntity(child);
        }

        detachFromParent(entity);
        mEntityRegistry.destroy(entity);
    }

    entt::entity Scene::createEntity(std::string_view name)
    {
        auto entity = mEntityRegistry.create();
        mEntityRegistry.emplace<TagComponent>(entity, std::string(name));
        return entity;
    }

    entt::entity Scene::createSunEntity(const SunData &data)
    {
        auto entity = createEntity(kSunName);
        mEntityRegistry.emplace<SunData>(
            entity,
            data
        );
        return entity;
    }

    void Scene::updateTransforms()
    {
        const auto walk = [this](this auto &&self, entt::entity entity, const glm::mat4 &parentWorld)-> void
        {
            auto world{parentWorld};

            if (auto *transform = mEntityRegistry.try_get<TransformComponent>(entity))
            {
                world = parentWorld * transform->transform.getModelMatrix();
                transform->worldMatrix = world;
            }

            if (const auto *hierarchy = mEntityRegistry.try_get<HierarchyComponent>(entity))
                for (auto child: hierarchy->children)
                    self(child, world);
        };

        for (auto entity : mEntityRegistry.view<TransformComponent>())
        {
            const auto* hierarchy{mEntityRegistry.try_get<HierarchyComponent>(entity)};
            if (!hierarchy || hierarchy->parent == entt::null)
                walk(entity, {1.f});
        }
    }
}
