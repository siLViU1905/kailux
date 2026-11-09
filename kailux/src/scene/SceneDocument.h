#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "components/entt/CameraComponent.h"
#include "components/gpu/DirectionalLightData.h"
#include "components/gpu/MeshMaterialData.h"
#include "components/math/Transform.h"
#include "physics/PhysicsTypes.h"
#include "audio/AudioRegistry.h"
#include "particles/ParticleTypes.h"

namespace kailux
{
    struct Version
    {
        uint8_t major{};
        uint8_t minor{};
    };
    constexpr Version kSceneVersion{5, 2};
    constexpr Version kPlaneMeshVersion{5, 2};

    constexpr uint32_t kNoEntity{~0u};

    using SunData = DirectionalLightData;

    struct MeshRecord
    {
        std::string path;
        MeshType    type{MeshType::Unknown};
    };

    struct MaterialTexturesRecord
    {
        std::string albedo;
        std::string normal;
        std::string roughness;
        std::string metallic;
        std::string ao;

        // same order as TextureRegistry::kTextureTypes
        std::array<std::string *, 5> Paths()
        {
            return {&albedo, &normal, &roughness, &metallic, &ao};
        }

        std::array<std::string, 5> ToPaths() const
        {
            return {albedo, normal, roughness, metallic, ao};
        }

        static MaterialTexturesRecord from(const std::array<std::string, 5> &paths)
        {
            return {paths[0], paths[1], paths[2], paths[3], paths[4]};
        }
    };

    struct PhysicsRecord
    {
        PhysicsBodyType type{PhysicsBodyType::Static};
        bool            canBecomeDynamic{false};
        float           mass{};
        bool            isSensor{};
    };

    struct PointLightRecord
    {
        float     intensity{1.f};
        float     range{10.f};
        glm::vec4 colorAndEnabled{1.f, 1.f, 1.f, 1.f};
    };

    struct AudioSourceRecord
    {
        std::string clip;
        float       gain{1.f};
        float       pitch{1.f};
        bool        loop{false};
        bool        playOnStart{true};
        bool        spatial{true};
        float       referenceDistance{1.f};
        float       maxDistance{50.f};
        float       rolloff{1.f};

        AudioSourceSettings ToSettings() const
        {
            return {gain, pitch, loop, playOnStart, spatial, referenceDistance, maxDistance, rolloff};
        }

        static AudioSourceRecord from(std::string_view clip, const AudioSourceSettings &settings)
        {
            return {
                std::string{clip},
                settings.gain,
                settings.pitch,
                settings.loop,
                settings.playOnStart,
                settings.spatial,
                settings.referenceDistance,
                settings.maxDistance,
                settings.rolloff
            };
        }
    };

    struct ScriptRecord
    {
        std::string name;
        nlohmann::json params{nlohmann::json::object()};
    };

    struct EntityRecord
    {
        uint32_t                               id{kNoEntity};
        uint32_t                               parent{kNoEntity};
        std::string                            name;
        std::optional<Transform>               transform{};
        std::optional<MeshRecord>              mesh;
        std::optional<MeshMaterialData>        material;
        std::optional<MaterialTexturesRecord>  materialTextures;
        std::optional<PointLightRecord>        light;
        std::optional<PhysicsRecord>           physics;
        std::optional<CameraComponent>         camera;
        std::optional<ScriptRecord>            script;
        std::optional<AudioSourceRecord>       audio;
        std::optional<ParticleEmitterSettings> particles;
    };

    struct SceneMeta
    {
        std::string           name{"Scene"};
        std::filesystem::path savePath{};
        uint32_t              meshNameCount{};
        uint32_t              lightNameCount{};
        uint32_t              cameraNameCount{};
        uint32_t              audioSourceNameCount{};
        uint32_t              particleEmitterNameCount{};
    };

    struct SceneDocument
    {
        Version                        version{kSceneVersion};
        SceneMeta                      meta{};
        SunData                        sun{};
        uint32_t                       mainCamera{kNoEntity};
        std::optional<CameraComponent> editorCamera{};
        std::vector<EntityRecord>      entities;
    };
}