#pragma once

#include "core/utilities/GlmJson.h"
#include "core/components/entt/CameraComponent.h"
#include "core/components/gpu/DirectionalLightData.h"
#include "core/components/gpu/MeshMaterialData.h"
#include "core/components/gpu/MeshTransformData.h"
#include "core/physics/PhysicsRegistry.h"

namespace kailux
{
    struct Version
    {
        uint8_t major{};
        uint8_t minor{};
    };
    constexpr Version kSceneVersion{2, 0};

    constexpr uint32_t kNoEntity{~0u};

    using SunData = DirectionalLightData;

    struct MeshRecord
    {
        std::string path;
        MeshType    type{MeshType::Unknown};
    };

    struct PhysicsRecord
    {
        PhysicsBodyType type{PhysicsBodyType::Static};
        bool            canBecomeDynamic{false};
    };

    struct PointLightRecord
    {
        float     intensity{1.f};
        float     range{10.f};
        glm::vec4 colorAndEnabled{1.f, 1.f, 1.f, 1.f};
    };

    struct EntityRecord
    {
        uint32_t                         id{kNoEntity};
        uint32_t                         parent{kNoEntity};
        std::string                      name;
        std::optional<MeshTransformData> transform{};
        std::optional<MeshRecord>        mesh;
        std::optional<MeshMaterialData>  material;
        std::optional<PointLightRecord>  light;
        std::optional<PhysicsRecord>     physics;
        std::optional<CameraComponent>   camera;
    };

    struct SceneMeta
    {
        std::string           name{"Scene"};
        std::filesystem::path savePath{};
        uint32_t              meshNameCount{};
        uint32_t              lightNameCount{};
    };

    struct SceneDocument
    {
        Version                   version{kSceneVersion};
        SceneMeta                 meta{};
        SunData                   sun{};
        uint32_t                  mainCamera{kNoEntity};
        std::vector<EntityRecord> entities;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MeshRecord, path, type)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhysicsRecord, type, canBecomeDynamic)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MeshTransformData, position, rotation, scale)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MeshMaterialData, albedoAndRoughness, pbrParams)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PointLightRecord, intensity, range, colorAndEnabled)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SunData, directionAndIntensity, colorAndEnabled)

    namespace details
    {
        template<typename T>
        void writeOptional(nlohmann::json &js, std::string_view key, const std::optional<T> &value)
        {
            if (value)
                js[std::string(key)] = *value;
        }

        template<typename T>
        void readOptional(const nlohmann::json &js, std::string_view key, std::optional<T> &value)
        {
            if (const auto it = js.find(std::string(key)); it != js.end() && !it->is_null())
                value = it->template get<T>();
            else
                value.reset();
        }
    }

    inline void to_json(nlohmann::json &js, const CameraComponent &component)
    {
        js = {
            {"isPrimary", component.isPrimary},
            {
                "transform", {
                        {"position", component.position},
                        {"forward", component.forward},
                        {"up", component.up},
                        {"right", component.right}
                }
            },
            {
                "settings", {
                        {"fov", component.fov},
                        {"zNear", component.zNear},
                        {"zFar", component.zFar},
                        {"exposure", component.exposure}
                }
            },
            {
                "input", {
                        {"yaw", component.yaw},
                        {"pitch", component.pitch},
                        {"speed", component.speed},
                        {"sensitivity", component.sensitivity},
                        {"focused", component.focused}
                }
            }
        };

    }
    inline void from_json(const nlohmann::json &js, CameraComponent &component)
    {
        component.isPrimary = js.value("isPrimary", component.isPrimary);
 
        if (const auto it = js.find("transform"); it != js.end() && it->is_object())
        {
            component.position = it->value("position", component.position);
            component.forward  = it->value("forward", component.forward);
            component.up       = it->value("up", component.up);
            component.right    = it->value("right", component.right);
        }
        if (const auto it = js.find("settings"); it != js.end() && it->is_object())
        {
            component.fov      = it->value("fov", component.fov);
            component.zNear    = it->value("zNear", component.zNear);
            component.zFar     = it->value("zFar", component.zFar);
            component.exposure = it->value("exposure", component.exposure);
        }
        if (const auto it = js.find("input"); it != js.end() && it->is_object())
        {
            component.yaw         = it->value("yaw", component.yaw);
            component.pitch       = it->value("pitch", component.pitch);
            component.speed       = it->value("speed", component.speed);
            component.sensitivity = it->value("sensitivity", component.sensitivity);
            component.focused     = it->value("focused", component.focused);
        }
    }

    inline void to_json(nlohmann::json &js, const SceneMeta &meta)
    {
        js = {
            {"name", meta.name},
            {"save_path", meta.savePath.generic_string()},
            {"mesh_name_count", meta.meshNameCount},
            {"light_name_count", meta.lightNameCount}
        };

    }
    inline void from_json(const nlohmann::json &js, SceneMeta &meta)
    {
        meta.name           = js.value("name", meta.name);
        meta.savePath       = std::filesystem::path(js.value("save_path", std::string{}));
        meta.meshNameCount  = js.value("mesh_name_count", meta.meshNameCount);
        meta.lightNameCount = js.value("light_name_count", meta.lightNameCount);
    }

    inline void to_json(nlohmann::json &js, const EntityRecord &record)
    {
        js = {
            {"id", record.id},
            {"name", record.name},
            {"transform", record.transform}
        };

        if (record.parent != kNoEntity)
            js["parent"] = record.parent;

        details::writeOptional(js, "mesh", record.mesh);
        details::writeOptional(js, "material", record.material);
        details::writeOptional(js, "light", record.light);
        details::writeOptional(js, "physics", record.physics);
        details::writeOptional(js, "camera", record.camera);
    }
    inline void from_json(const nlohmann::json &js, EntityRecord &record)
    {
        record.id        = js.value("id", kNoEntity);
        record.parent    = js.value("parent", kNoEntity);
        record.name      = js.value("name", std::string{});
        record.transform = js.value("transform", MeshTransformData{});
        record.camera    = js.value("camera", CameraComponent{});

        details::readOptional(js, "mesh", record.mesh);
        details::readOptional(js, "material", record.material);
        details::readOptional(js, "light", record.light);
        details::readOptional(js, "physics", record.physics);
        details::readOptional(js, "camera", record.camera);

        record.physics.reset();
        if (const auto it = js.find("physics"); it != js.end() && !it->is_null())
            record.physics = it->get<PhysicsRecord>();
    }

    inline void to_json(nlohmann::json &js, const SceneDocument &document)
    {
        js = {
            {"version_major", document.version.major},
            {"version_minor", document.version.minor},
            {"meta", document.meta},
            {"sun", document.sun},
            {"main_camera", document.mainCamera},
            {"entities", document.entities}
        };

    }
    inline void from_json(const nlohmann::json &js, SceneDocument &document)
    {
        document.version.major = js.value("version_major", 0u);
        document.version.minor = js.value("version_minor", 0u);
        document.meta          = js.value("meta", SceneMeta{});
        document.sun           = js.value("sun", SunData{});
        document.mainCamera    = js.value("main_camera", kNoEntity);
 
        document.entities.clear();
        if (const auto it = js.find("entities"); it != js.end() && it->is_array())
        {
            document.entities.reserve(it->size());
            for (const auto &entry : *it)
                document.entities.push_back(entry.get<EntityRecord>());
        }
    }

}
