#pragma once

#include <nlohmann/json.hpp>

#include "SceneDocument.h"
#include "utilities/GlmJson.h"

namespace kailux
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MeshRecord, path, type)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhysicsRecord, type, canBecomeDynamic, mass, isSensor)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Transform, position, rotation, scale)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MeshMaterialData, albedoAndRoughness, pbrParams)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PointLightRecord, intensity, range, colorAndEnabled)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SunData, directionAndIntensity, colorAndEnabled)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AudioSourceRecord, clip, gain, pitch, loop, playOnStart, spatial,
                                                    referenceDistance, maxDistance, rolloff)

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
                value = it->get<T>();
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

    inline void to_json(nlohmann::json &js, const ScriptRecord &record)
    {
        js = {{"name", record.name}};
        if (record.params.is_object() && !record.params.empty())
            js["params"] = record.params;
    }
    inline void from_json(const nlohmann::json &js, ScriptRecord &record)
    {
        if (js.is_string())
        {
            record.name = js.get<std::string>();
            record.params = nlohmann::json::object();
            return;
        }

        record.name = js.value("name", std::string{});
        if (const auto it{js.find("params")}; it != js.end() && it->is_object())
            record.params = *it;
        else
            record.params = nlohmann::json::object();
    }

    inline void to_json(nlohmann::json &js, const SceneMeta &meta)
    {
        js = {
            {"name", meta.name},
            {"save_path", meta.savePath.generic_string()},
            {"mesh_name_count", meta.meshNameCount},
            {"light_name_count", meta.lightNameCount},
            {"camera_name_count", meta.cameraNameCount},
            {"audio_source_name_count", meta.audioSourceNameCount}
        };

    }
    inline void from_json(const nlohmann::json &js, SceneMeta &meta)
    {
        meta.name            = js.value("name", meta.name);
        meta.savePath        = std::filesystem::path(js.value("save_path", std::string{}));
        meta.meshNameCount   = js.value("mesh_name_count", meta.meshNameCount);
        meta.lightNameCount  = js.value("light_name_count", meta.lightNameCount);
        meta.cameraNameCount = js.value("camera_name_count", meta.cameraNameCount);
        meta.audioSourceNameCount = js.value("audio_source_name_count", meta.audioSourceNameCount);
    }

    inline void to_json(nlohmann::json &js, const EntityRecord &record)
    {
        js = {
            {"id", record.id},
            {"name", record.name}
        };

        if (record.parent != kNoEntity)
            js["parent"] = record.parent;

        details::writeOptional(js, "transform", record.transform);
        details::writeOptional(js, "mesh", record.mesh);
        details::writeOptional(js, "material", record.material);
        details::writeOptional(js, "light", record.light);
        details::writeOptional(js, "physics", record.physics);
        details::writeOptional(js, "camera", record.camera);
        details::writeOptional(js, "script", record.script);
        details::writeOptional(js, "audio", record.audio);
    }
    inline void from_json(const nlohmann::json &js, EntityRecord &record)
    {
        record.id     = js.value("id", kNoEntity);
        record.parent = js.value("parent", kNoEntity);
        record.name   = js.value("name", std::string{});

        details::readOptional(js, "transform", record.transform);
        details::readOptional(js, "mesh", record.mesh);
        details::readOptional(js, "material", record.material);
        details::readOptional(js, "light", record.light);
        details::readOptional(js, "physics", record.physics);
        details::readOptional(js, "camera", record.camera);
        details::readOptional(js, "script", record.script);
        details::readOptional(js, "audio", record.audio);
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

        details::writeOptional(js, "editor_camera", document.editorCamera);
    }
    inline void from_json(const nlohmann::json &js, SceneDocument &document)
    {
        document.version.major = js.value("version_major", 0u);
        document.version.minor = js.value("version_minor", 0u);
        document.meta          = js.value("meta", SceneMeta{});
        document.sun           = js.value("sun", SunData{});
        document.mainCamera    = js.value("main_camera", kNoEntity);

        details::readOptional(js, "editor_camera", document.editorCamera);

        document.entities.clear();
        if (const auto it = js.find("entities"); it != js.end() && it->is_array())
        {
            document.entities.reserve(it->size());
            for (const auto &entry : *it)
                document.entities.push_back(entry.get<EntityRecord>());
        }
    }
}