#pragma once

#include <filesystem>
#include <optional>
#include "CameraRef.h"
#include "Entity.h"

namespace kailux
{
    struct ScriptServices;

    struct RaycastHit
    {
        Entity    entity;
        glm::vec3 point{0.f};
        glm::vec3 normal{0.f};
        float     distance{};
    };

    class World
    {
    public:
        Entity Find(std::string_view name) const;
        Entity Spawn(std::string_view name, const Transform& transform = {});
        Entity SpawnPrimitive(std::string_view name, Primitive shape, const Transform& transform = {},
                              std::optional<BodySettings> body = std::nullopt);
        Entity SpawnMesh(const std::filesystem::path& path, std::string_view name, const Transform& transform = {},
                         std::optional<BodySettings> body = std::nullopt);

        bool AddBody(Entity entity, const BodySettings& settings);
        void Destroy(Entity entity);

        CameraRef GetMainCamera() const;

        std::optional<RaycastHit> Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                                          Entity ignore = {}) const;

        bool LoadScene(const std::filesystem::path& path);

        void SetBuiltinCameraControl(bool enabled);

    private:
        void Bind(Scene& scene, ScriptServices& services);
        void SetViewportSize(glm::ivec2 size);
        void FlushDestroyed();
        void Clear();

        Scene*                    pScene{};
        ScriptServices*           pServices{};
        glm::ivec2                mViewportSize{0};
        std::vector<entt::entity> mPendingDestroy;

        friend class ScriptSystem;
    };
}