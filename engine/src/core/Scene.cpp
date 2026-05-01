#include "Scene.h"

#include "components/entt/CameraComponent.h"
#include "components/entt/MeshComponent.h"
#include "components/entt/MaterialComponent.h"
#include "components/entt/TagComponent.h"
#include "components/gpu/CameraData.h"
#include <nlohmann/json.hpp>

namespace kailux
{
    Scene::Scene() : m_Name("Scene"),
                     m_MainCameraEntity(entt::null),
                     m_Sun(entt::null),
                     m_Ambient(SceneData().ambient),
                     m_MeshEntityNameCount(0)
    {
        m_Sun = createSunEntity({});
    }

    Scene::Scene(Scene &&other) noexcept : m_Name(std::move(other.m_Name)),
                                           m_EntityRegistry(std::move(other.m_EntityRegistry)),
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
            m_Name = std::move(other.m_Name);
            m_EntityRegistry = std::move(other.m_EntityRegistry);
            m_MainCameraEntity = other.m_MainCameraEntity;
            m_Sun = other.m_Sun;
            m_Ambient = other.m_Ambient;
            m_MeshEntityNameCount = other.m_MeshEntityNameCount;
        }
        return *this;
    }

    Scene Scene::create(std::string_view name)
    {
        Scene scene;
        scene.m_Name = name;
        return scene;
    }

    entt::entity Scene::createCameraEntity(std::string_view name, bool isPrimary, int width, int height)
    {
        auto entity = createEntity(name);
        auto component = m_EntityRegistry.emplace<CameraComponent>(
            entity,
            isPrimary
        );
        m_EntityRegistry.emplace<CameraData>(
            entity,
            Camera::get_projection(component, width, height),
            Camera::get_view(component),
            glm::vec4(component.position, component.exposure)
        );
        return entity;
    }

    entt::entity Scene::createMeshEntity(
        std::string_view name,
        MeshHandle meshHandle,
        std::string_view path,
        MeshType type,
        TextureSetHandle textureSetHandle,
        const MeshTransformData &transform,
        const MeshMaterialData &material
    )
    {
        auto entity = createEntity(name);
        m_EntityRegistry.emplace<MeshComponent>(
            entity,
            meshHandle,
            path.data(),
            type
        );
        m_EntityRegistry.emplace<MaterialComponent>(
            entity,
            textureSetHandle
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

    std::string_view Scene::getName() const
    {
        return m_Name;
    }

    std::string Scene::getMeshEntityName()
    {
        return "Mesh" + std::to_string(m_MeshEntityNameCount++);
    }

    std::string Scene::serialize() const
    {
        nlohmann::json js;

        const auto &sunData = m_EntityRegistry.get<SunData>(m_Sun);
        const auto &cameraComponent = m_EntityRegistry.get<CameraComponent>(m_MainCameraEntity);
        js["Scene"] = {
            {"name", m_Name},
            {"ambient", {m_Ambient.x, m_Ambient.y, m_Ambient.z, m_Ambient.w}},
            {"mesh_name_count", m_MeshEntityNameCount},
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
        auto meshView = m_EntityRegistry.view<TagComponent, MeshComponent, MeshTransformData, MeshMaterialData>();
        meshView.each([&js](const auto &tag, const auto &mesh, const auto &transform, const auto &material)
        {
            nlohmann::json meshEntry;

            meshEntry["name"] = tag.name;

            meshEntry["path"] = mesh.path;
            meshEntry["type"] = static_cast<uint8_t>(mesh.type);

            meshEntry["transform"] = {
                {"position", {transform.position.x, transform.position.y, transform.position.z}},
                {"rotation", {transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w}},
                {"scale", {transform.scale.x, transform.scale.y, transform.scale.z}}
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

            js["Mesh"].push_back(meshEntry);
        });

        return js.dump(3);
    }

    nlohmann::json Scene::deserialize(std::string_view content, int windowWidth, int windowHeight)
    {
        auto js = nlohmann::json::parse(content);
        auto &sceneJs = js["Scene"];

        m_EntityRegistry.clear();
        m_Sun = createSunEntity({});

        m_Name = sceneJs.value("name", "Scene");
        m_MeshEntityNameCount = sceneJs.value("mesh_name_count", 0);

        if (sceneJs.contains("ambient"))
        {
            m_Ambient.x = sceneJs["ambient"][0];
            m_Ambient.y = sceneJs["ambient"][1];
            m_Ambient.z = sceneJs["ambient"][2];
            m_Ambient.w = sceneJs["ambient"][3];
        }

        auto &sunJs = sceneJs["sun"];
        auto &sunData = m_EntityRegistry.get<SunData>(m_Sun);

        sunData.directionAndIntensity.x = sunJs["direction"][0];
        sunData.directionAndIntensity.y = sunJs["direction"][1];
        sunData.directionAndIntensity.z = sunJs["direction"][2];
        sunData.directionAndIntensity.w = sunJs["intensity"];

        sunData.colorAndEnabled.x = sunJs["color"][0];
        sunData.colorAndEnabled.y = sunJs["color"][1];
        sunData.colorAndEnabled.z = sunJs["color"][2];
        sunData.colorAndEnabled.w = sunJs["enabled"];

        auto &camJs = sceneJs["camera"];
        m_MainCameraEntity = createCameraEntity("MainCamera", camJs["isPrimary"], windowWidth, windowHeight);
        auto &cameraComponent = m_EntityRegistry.get<CameraComponent>(m_MainCameraEntity);

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
