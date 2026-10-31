#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <variant>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "Core.h"
#include "components/math/Transform.h"
#include "physics/PhysicsRegistry.h"

namespace kailux::scene_commands
{
    struct SpawnPrimitive
    {
        MeshType type{MeshType::Cube};
    };

    struct ImportMesh
    {
        std::filesystem::path path;
    };

    struct SpawnLight
    {
        LightType type{LightType::Point};
    };

    struct SpawnCamera
    {
        glm::ivec2 extent{};
    };

    struct SpawnAudioSource {};

    struct DeleteEntity
    {
        entt::entity entity{entt::null};
    };

    struct RenameEntity
    {
        entt::entity entity{entt::null};
        std::string  name;
    };

    struct SetTransform
    {
        entt::entity entity{entt::null};
        Transform    transform;
        bool         syncBodyScale{};
    };

    struct AddPhysics
    {
        entt::entity           entity{entt::null};
        PhysicsCreationOptions options;
    };

    struct AddAudioSource
    {
        entt::entity entity{entt::null};
    };

    struct RemoveAudioSource
    {
        entt::entity entity{entt::null};
    };

    struct SetBodyType
    {
        entt::entity    entity{entt::null};
        PhysicsBodyType type{PhysicsBodyType::Static};
    };

    struct SetBodyMass
    {
        entt::entity entity{entt::null};
        float        mass{};
        bool         commit{};
    };

    struct LoadScene
    {
        std::filesystem::path                 path;
        std::move_only_function<void()>       onStaged;
    };
}

namespace kailux
{
    using SceneCommand = std::variant<
         scene_commands::SpawnPrimitive,
         scene_commands::ImportMesh,
         scene_commands::SpawnLight,
         scene_commands::SpawnCamera,
         scene_commands::SpawnAudioSource,
         scene_commands::DeleteEntity,
         scene_commands::RenameEntity,
         scene_commands::SetTransform,
         scene_commands::AddPhysics,
         scene_commands::AddAudioSource,
         scene_commands::RemoveAudioSource,
         scene_commands::SetBodyType,
         scene_commands::SetBodyMass,
         scene_commands::LoadScene
     >;
}