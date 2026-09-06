#include "Scene.h"

#include "../components/entt/CameraComponent.h"
#include "../components/entt/MeshComponent.h"
#include "../components/entt/MaterialComponent.h"
#include "../components/entt/TagComponent.h"
#include "../components/entt/BuiltinCamera.h"
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
                                           mSceneCameraEntity(other.mSceneCameraEntity),
                                           mSimulationCameraEntity(other.mSimulationCameraEntity),
                                           mSun(other.mSun),
                                           mMeshEntityNameCount(other.mMeshEntityNameCount),
                                           mLightEntityNameCount(other.mLightEntityNameCount),
                                           mCameraEntityNameCount(other.mCameraEntityNameCount)
    {
    }

    Scene &Scene::operator=(Scene &&other) noexcept
    {
        if (this != &other)
        {
            mName = std::move(other.mName);
            mSavePath = std::move(other.mSavePath);
            mEntityRegistry = std::move(other.mEntityRegistry);
            mSceneCameraEntity = other.mSceneCameraEntity;
            mSimulationCameraEntity = other.mSimulationCameraEntity;
            mSun = other.mSun;
            mMeshEntityNameCount = other.mMeshEntityNameCount;
            mLightEntityNameCount = other.mLightEntityNameCount;
            mCameraEntityNameCount = other.mCameraEntityNameCount;
        }
        return *this;
    }

    Scene Scene::create(std::string_view name, const Window &window)
    {
        Scene scene;
        scene.mName = name;
        scene.mSun = scene.CreateSunEntity({});
        scene.CreateCameras(window);
        return scene;
    }

    void Scene::Update()
    {
        UpdateTransforms();
    }

    Scene::CreateResult Scene::CreateCameraEntity(std::string_view name, const GizmoComponent &component, bool isPrimary)
    {
        if (mEntityRegistry.view<CameraComponent>(entt::exclude<BuiltinCamera>).size_hint() >= details::kMaxCameras)
            return std::unexpected{"The maximum number of cameras has been reached"};

        auto entity = CreateEntity(name);
        AttachCamera(entity, component, {isPrimary});
        return entity;
    }

    entt::entity Scene::CreateBuiltinCameraEntity(std::string_view name)
    {
        auto entity = CreateEntity(name);
        AttachBuiltinCamera(entity);
        return entity;
    }

    Scene::CreateResult Scene::CreateMeshEntity(
        std::string_view name,
        const MeshComponent &component,
        MaterialHandle materialHandle,
        const MeshTransformData &transform,
        const MeshMaterialData &material,
        entt::entity parent
    )
    {
        if (mEntityRegistry.view<MeshComponent>().size() >= details::kMaxMeshes)
            return std::unexpected{"The maximum number of meshes has been reached"};

        auto entity = CreateEntity(name);
        SetLocalTransform(entity, transform);

        if (!AttachMesh(entity, component, materialHandle, material))
        {
            DestroyEntity(entity);
            return std::unexpected{std::format("Cannot attach mesh component to {}", name)};
        }

        if (parent != entt::null)
            SetParent(entity, parent);

        return entity;
    }

    entt::entity Scene::CreateParentEntity(std::string_view name)
    {
        return CreateEntity(name);
    }

    Scene::CreateResult Scene::CreatePointLightEntity(std::string_view name, const GizmoComponent &component,
                                                      const glm::vec3 &position)
    {
        if (mEntityRegistry.view<PointLightData>().size() >= details::kMaxPointLights)
            return std::unexpected{"The maximum number of point lights has been reached"};
        auto entity = CreateEntity(name);

        AttachPointLight(entity, component, {});

        MeshTransformData transform;
        transform.position = position;
        mEntityRegistry.emplace<TransformComponent>(
            entity,
            transform,
            glm::mat4(1.f),
            transform.GetModelMatrix()
        );
        mEntityRegistry.emplace<HierarchyComponent>(entity);

        return entity;
    }

    entt::registry &Scene::GetEntityRegistry()
    {
        return mEntityRegistry;
    }

    const entt::registry &Scene::GetEntityRegistry() const
    {
        return mEntityRegistry;
    }

    entt::entity Scene::GetSceneCamera() const
    {
        return mSceneCameraEntity;
    }

    entt::entity Scene::GetSimulationCamera() const
    {
        return mSimulationCameraEntity;
    }

    void Scene::SetMainCamera(entt::entity camera)
    {
        mSceneCameraEntity = camera;
    }

    entt::entity Scene::GetSun() const
    {
        return mSun;
    }

    SceneData Scene::GetData() const
    {
        return {GetLightData()};
    }

    LightsData Scene::GetLightData() const
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

    std::string_view Scene::GetName() const
    {
        return mName;
    }

    std::string Scene::GetMeshEntityName()
    {
        return std::format("Mesh{}", mMeshEntityNameCount++);
    }

    std::string Scene::GetLightEntityName()
    {
        return std::format("Light{}", mLightEntityNameCount++);
    }

    std::string Scene::GetCameraEntityName()
    {
        return std::format("Camera{}", mCameraEntityNameCount++);
    }

    void Scene::SetSavePath(const std::filesystem::path &path)
    {
        mSavePath = path;
    }

    const std::filesystem::path & Scene::GetSavePath() const
    {
        return mSavePath;
    }

    void Scene::SetMeta(const SceneMeta &meta)
    {
        mName                 = meta.name;
        mSavePath             = meta.savePath;
        mMeshEntityNameCount  = meta.meshNameCount;
        mLightEntityNameCount = meta.lightNameCount;
    }

    SceneMeta Scene::GetMeta() const
    {
        return {
            mName,
            mSavePath,
            mMeshEntityNameCount,
            mLightEntityNameCount
        };
    }

    bool Scene::AttachMesh(entt::entity entity, const MeshComponent &component, MaterialHandle materialHandle,
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

    bool Scene::AttachMeshSource(entt::entity entity, const MeshSourceComponent &source)
    {
        if (mEntityRegistry.valid(entity))
        {
            mEntityRegistry.emplace_or_replace<MeshSourceComponent>(entity, source);
            return true;
        }
        return false;
    }

    bool Scene::AttachPointLight(entt::entity entity, const GizmoComponent &component, const PointLightRecord &light)
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

    void Scene::AttachPhysics(entt::entity entity, PhysicsComponent component)
    {
        if (mEntityRegistry.valid(entity))
        {
            mEntityRegistry.emplace_or_replace<PhysicsComponent>(entity, component);
            mEntityRegistry.emplace<PhysicsControlComponent>(entity);
        }
    }

    void Scene::AttachCamera(entt::entity entity, const GizmoComponent &component, const CameraComponent &camera)
    {
        if (!mEntityRegistry.valid(entity))
            return;

        mEntityRegistry.emplace_or_replace<GizmoComponent>(entity, component);
        mEntityRegistry.emplace_or_replace<CameraComponent>(entity, camera);
    }

    void Scene::AttachBuiltinCamera(entt::entity entity)
    {
        if (!mEntityRegistry.valid(entity))
            return;

        mEntityRegistry.emplace_or_replace<CameraComponent>(entity);
        mEntityRegistry.emplace_or_replace<BuiltinCamera>(entity);
    }

    void Scene::SetLocalTransform(entt::entity entity, const MeshTransformData &transform)
    {
        if (!mEntityRegistry.valid(entity))
            return;

        auto& component = mEntityRegistry.emplace_or_replace<TransformComponent>(entity);
        component.transform = transform;
        component.worldMatrix = transform.GetModelMatrix();
    }

    void Scene::SetParent(entt::entity child, entt::entity parent)
    {
        if (child == parent || !mEntityRegistry.valid(child))
            return;

        DetachFromParent(child);
        if (parent == entt::null || !mEntityRegistry.valid(parent))
            return;

        mEntityRegistry.get_or_emplace<HierarchyComponent>(child).parent = parent;
        mEntityRegistry.get_or_emplace<HierarchyComponent>(parent).children.push_back(child);
    }
    void Scene::DetachFromParent(entt::entity child)
    {
        auto *hierarchy = mEntityRegistry.try_get<HierarchyComponent>(child);
        if (!hierarchy || hierarchy->parent == entt::null)
            return;

        if (auto *old = mEntityRegistry.try_get<HierarchyComponent>(hierarchy->parent))
            std::erase(old->children, child);

        hierarchy->parent = entt::null;
    }

    void Scene::DestroyEntity(entt::entity entity)
    {
        if (!mEntityRegistry.valid(entity))
            return;

        if (const auto *hierarchy = mEntityRegistry.try_get<HierarchyComponent>(entity))
        {
            auto children = hierarchy->children;
            for (auto child : children)
                DestroyEntity(child);
        }

        DetachFromParent(entity);
        mEntityRegistry.destroy(entity);
    }

    entt::entity Scene::CreateEntity(std::string_view name)
    {
        auto entity = mEntityRegistry.create();
        mEntityRegistry.emplace<TagComponent>(entity, std::string(name));
        return entity;
    }

    entt::entity Scene::CreateSunEntity(const SunData &data)
    {
        auto entity = CreateEntity(kSunName);
        mEntityRegistry.emplace<SunData>(
            entity,
            data
        );
        return entity;
    }

    void Scene::CreateCameras(const Window &window)
    {
        mSceneCameraEntity = CreateBuiltinCameraEntity("SceneCamera");
        mSimulationCameraEntity = CreateBuiltinCameraEntity("SimulationCamera");
    }

    void Scene::UpdateTransforms()
    {
        const auto walk = [this](this auto &&self, entt::entity entity, const glm::mat4 &parentWorld)-> void
        {
            auto world{parentWorld};

            if (auto *transform = mEntityRegistry.try_get<TransformComponent>(entity))
            {
                world = parentWorld * transform->transform.GetModelMatrix();
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
