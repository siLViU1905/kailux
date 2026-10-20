#pragma once

#include <filesystem>
#include <functional>
#include <optional>

#include "Input.h"
#include "Time.h"
#include "World.h"
#include "physics/PhysicsRegistry.h"

namespace kailux
{
    struct ScriptServices
    {
        std::move_only_function<entt::entity(
                Scene &,
                std::string_view, MeshType,
                const Transform &
            )
        > spawnPrimitive;

        std::move_only_function<entt::entity(
                Scene &,
                const std::filesystem::path &,
                std::string_view,
                const Transform &,
                std::optional<PhysicsCreationOptions>)
        > spawnMesh;

        std::move_only_function<bool(
                Scene &,
                entt::entity,
                PhysicsCreationOptions)
        > addBody;

        std::move_only_function<bool(const std::filesystem::path &)> loadScene;

        std::move_only_function<void(bool)> setBuiltinCameraControl;
    };

    struct ScriptContext
    {
        World          world;
        Input          input;
        Time           time;
        ScriptServices services;
    };
}