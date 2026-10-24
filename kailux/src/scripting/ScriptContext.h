#pragma once

#include <filesystem>
#include <optional>

#include "Input.h"
#include "Time.h"
#include "World.h"
#include "physics/PhysicsRegistry.h"

namespace kailux
{
    class Engine;

    class ScriptServices
    {
    public:
        explicit ScriptServices(Engine* engine);

        entt::entity SpawnPrimitive(Scene &scene, std::string_view name, MeshType type, const Transform &transform) const;
        entt::entity SpawnMesh(Scene &scene,
                               const std::filesystem::path &path,
                               std::string_view name,
                               const Transform & transform,
                               std::optional<PhysicsCreationOptions> physics) const;

        bool AddBody(Scene& scene, entt::entity entity, PhysicsCreationOptions options) const;

        bool LoadScene(const std::filesystem::path& path) const;

        void SetBuiltinCameraControl(bool enable) const;

        bool  PlayOneShot(const std::filesystem::path& path, std::optional<glm::vec3> position, float gain) const;
        void  SetMasterVolume(float volume) const;
        float GetMasterVolume() const;

    private:
        Engine* pEngine{};
    };

    struct ScriptContext
    {
        World          world;
        Input          input;
        Time           time;
        ScriptServices services{nullptr};
    };
}