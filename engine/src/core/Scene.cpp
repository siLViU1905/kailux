#include "Scene.h"

#include "components/entt/CameraComponent.h"
#include "components/entt/MeshComponent.h"
#include "components/entt/MaterialComponent.h"
#include "components/entt/TagComponent.h"
#include "components/gpu/CameraData.h"
#include <nlohmann/json.hpp>
#include "components/entt/HierarchyComponent.h"
#include "components/entt/PhysicsComponent.h"
#include "components/gpu/TransformComponent.h"

namespace kailux
{
    Scene::Scene() : mName("Scene"),
                     mMainCameraEntity(entt::null),
                     mSun(entt::null),
                     mMeshEntityNameCount(0)
    {
        mSun = createSunEntity({});
    }

    Scene::Scene(Scene &&other) noexcept : mName(std::move(other.mName)),
                                           mEntityRegistry(std::move(other.mEntityRegistry)),
                                           mMainCameraEntity(other.mMainCameraEntity),
                                           mSun(other.mSun),
                                           mMeshEntityNameCount(other.mMeshEntityNameCount)
    {
    }

    Scene &Scene::operator=(Scene &&other) noexcept
    {
        if (this != &other)
        {
            mName = std::move(other.mName);
            mEntityRegistry = std::move(other.mEntityRegistry);
            mMainCameraEntity = other.mMainCameraEntity;
            mSun = other.mSun;
            mMeshEntityNameCount = other.mMeshEntityNameCount;
        }
        return *this;
    }

    Scene Scene::create(std::string_view name)
    {
        Scene scene;
        scene.mName = name;
        return scene;
    }

    void Scene::update()
    {
        updateTransforms();
    }

    entt::entity Scene::createCameraEntity(std::string_view name, bool isPrimary, int width, int height)
    {
        auto entity = createEntity(name);
        auto component = mEntityRegistry.emplace<CameraComponent>(
            entity,
            isPrimary
        );
        mEntityRegistry.emplace<CameraData>(
            entity,
            Camera::get_projection(component, width, height),
            Camera::get_view(component),
            glm::vec4(component.position, component.exposure)
        );
        return entity;
    }

    entt::entity Scene::createMeshEntity(
        std::string_view name,
        const MeshComponent &component,
        TextureSetHandle textureSetHandle,
        const MeshTransformData &transform,
        const MeshMaterialData &material,
        entt::entity parent
    )
    {
        auto entity = createEntity(name);
        mEntityRegistry.emplace<MeshComponent>(
            entity,
            component
        );
        mEntityRegistry.emplace<MaterialComponent>(
            entity,
            textureSetHandle
        );
        mEntityRegistry.emplace<TransformComponent>(
            entity,
            transform,
            transform.getModelMatrix(),
            glm::mat4(1.f)
        );
        mEntityRegistry.emplace<MeshMaterialData>
        (
            entity,
            material
        );
        mEntityRegistry.emplace<HierarchyComponent>(entity, parent);

        if (parent != entt::null)
            mEntityRegistry.get<HierarchyComponent>(parent).children.push_back(entity);

        return entity;
    }

    entt::entity Scene::createParentEntity(std::string_view name)
    {
        return createEntity(name);
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
        const auto &sunData = mEntityRegistry.get<SunData>(mSun);
        return {sunData};
    }

    std::string_view Scene::getName() const
    {
        return mName;
    }

    std::string Scene::getMeshEntityName()
    {
        return "Mesh" + std::to_string(mMeshEntityNameCount++);
    }

    std::string Scene::serialize() const
    {
        nlohmann::json js;

        const auto &sunData = mEntityRegistry.get<SunData>(mSun);
        const auto &cameraComponent = mEntityRegistry.get<CameraComponent>(mMainCameraEntity);
        js["Scene"] = {
            {"name", mName},
            {"mesh_name_count", mMeshEntityNameCount},
            {
                "sun", {
                    {
                        "direction",
                        {
                            sunData.directionAndIntensity.x, sunData.directionAndIntensity.y,
                            sunData.directionAndIntensity.z
                        }
                    },
                    {"intensity", sunData.directionAndIntensity.w},
                    {"color", {sunData.colorAndEnabled.x, sunData.colorAndEnabled.y, sunData.colorAndEnabled.z}},
                    {"enabled", sunData.colorAndEnabled.w}
                }
            },
            {
                "camera", {
                    {"isPrimary", cameraComponent.isPrimary},
                    {
                        "transform", {
                            {
                                "position",
                                {cameraComponent.position.x, cameraComponent.position.y, cameraComponent.position.z}
                            },
                            {
                                "forward",
                                {cameraComponent.forward.x, cameraComponent.forward.y, cameraComponent.forward.z}
                            },
                            {"up", {cameraComponent.up.x, cameraComponent.up.y, cameraComponent.up.z}},
                            {"right", {cameraComponent.right.x, cameraComponent.right.y, cameraComponent.right.z}}
                        }
                    },
                    {
                        "settings", {
                            {"fov", cameraComponent.fov},
                            {"zNear", cameraComponent.zNear},
                            {"zFar", cameraComponent.zFar},
                            {"exposure", cameraComponent.exposure}
                        }
                    },
                    {
                        "input", {
                            {"yaw", cameraComponent.yaw},
                            {"pitch", cameraComponent.pitch},
                            {"speed", cameraComponent.speed},
                            {"sensitivity", cameraComponent.sensitivity},
                            {"focused", cameraComponent.focused}
                        }
                    }
                }
            }
        };

        js["Mesh"] = nlohmann::json::array();
        auto meshView = mEntityRegistry.view<TagComponent, MeshComponent, TransformComponent, MeshMaterialData, PhysicsComponent>();
        meshView.each([&js](const auto &tag, const auto &mesh, const auto &transformComponent, const auto &material, auto physics)
        {
            nlohmann::json meshEntry;

            meshEntry["name"] = tag.name;

            meshEntry["path"] = mesh.path;
            meshEntry["type"] = static_cast<uint8_t>(mesh.type);

            meshEntry["transform"] = {
                {"position", {transformComponent.transform.position.x, transformComponent.transform.position.y, transformComponent.transform.position.z}},
                {"rotation", {transformComponent.transform.rotation.x, transformComponent.transform.rotation.y, transformComponent.transform.rotation.z, transformComponent.transform.rotation.w}},
                {"scale", {transformComponent.transform.scale.x, transformComponent.transform.scale.y, transformComponent.transform.scale.z}}
            };

            meshEntry["material"] = {
                {
                    "albedo",
                    {material.albedoAndRoughness.x, material.albedoAndRoughness.y, material.albedoAndRoughness.z}
                },
                {"roughness", material.albedoAndRoughness.w},
                {"metallic", material.pbrParams.x},
                {"ao", material.pbrParams.y}
            };

            meshEntry["physics"] = {
                {"body", static_cast<uint8_t>(physics.type)}
            };

            js["Mesh"].push_back(meshEntry);
        });

        return js.dump(3);
    }

    nlohmann::json Scene::deserialize(std::string_view content, int windowWidth, int windowHeight)
    {
        auto js = nlohmann::json::parse(content);
        auto &sceneJs = js["Scene"];

        mEntityRegistry.clear();
        mSun = createSunEntity({});

        mName = sceneJs.value("name", "Scene");
        mMeshEntityNameCount = sceneJs.value("mesh_name_count", 0);

        auto &sunJs = sceneJs["sun"];
        auto &sunData = mEntityRegistry.get<SunData>(mSun);

        sunData.directionAndIntensity.x = sunJs["direction"][0];
        sunData.directionAndIntensity.y = sunJs["direction"][1];
        sunData.directionAndIntensity.z = sunJs["direction"][2];
        sunData.directionAndIntensity.w = sunJs["intensity"];

        sunData.colorAndEnabled.x = sunJs["color"][0];
        sunData.colorAndEnabled.y = sunJs["color"][1];
        sunData.colorAndEnabled.z = sunJs["color"][2];
        sunData.colorAndEnabled.w = sunJs["enabled"];

        auto &camJs = sceneJs["camera"];
        mMainCameraEntity = createCameraEntity("MainCamera", camJs["isPrimary"], windowWidth, windowHeight);
        auto &cameraComponent = mEntityRegistry.get<CameraComponent>(mMainCameraEntity);

        auto &transJs = camJs["transform"];
        cameraComponent.position = {transJs["position"][0], transJs["position"][1], transJs["position"][2]};
        cameraComponent.forward = {transJs["forward"][0], transJs["forward"][1], transJs["forward"][2]};
        cameraComponent.up = {transJs["up"][0], transJs["up"][1], transJs["up"][2]};
        cameraComponent.right = {transJs["right"][0], transJs["right"][1], transJs["right"][2]};

        auto &setJs = camJs["settings"];
        cameraComponent.fov = setJs["fov"];
        cameraComponent.zNear = setJs["zNear"];
        cameraComponent.zFar = setJs["zFar"];
        cameraComponent.exposure = setJs["exposure"];

        auto &inJs = camJs["input"];
        cameraComponent.yaw = inJs["yaw"];
        cameraComponent.pitch = inJs["pitch"];
        cameraComponent.speed = inJs["speed"];
        cameraComponent.sensitivity = inJs["sensitivity"];
        cameraComponent.focused = inJs["focused"];

        return js;
    }

    entt::entity Scene::createEntity(std::string_view name)
    {
        entt::entity entity = mEntityRegistry.create();
        mEntityRegistry.emplace<TagComponent>(entity, name.data());
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
        auto updateHierarchy = [&](auto& self, auto entity, const glm::mat4& parentWorldMatrix)-> void
        {
            if (auto* transform = mEntityRegistry.try_get<TransformComponent>(entity))
            {
                transform->worldMatrix = parentWorldMatrix * transform->transform.getModelMatrix() * transform->submeshLocalMatrix;

                if (auto* hierarchy = mEntityRegistry.try_get<HierarchyComponent>(entity))
                    for (auto child : hierarchy->children)
                        self(self, child, transform->worldMatrix);
            }
        };

        auto view = mEntityRegistry.view<HierarchyComponent>();
        for (auto entity : view)
        {
            const auto& hierarchy = view.get<HierarchyComponent>(entity);
            if (hierarchy.parent == entt::null)
                updateHierarchy(updateHierarchy, entity, {1.f});
        }
    }
}
