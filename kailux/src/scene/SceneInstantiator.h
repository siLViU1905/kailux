#pragma once
#include <expected>

#include "SceneDocument.h"
#include "gizmo/GizmoRegistry.h"
#include "physics/PhysicsRegistry.h"

namespace kailux
{
    class Scene;

    struct SceneLoadContext
    {
        std::reference_wrapper<GizmoRegistry>   gizmoRegistry;
        int                                     windowWidth{};
        int                                     windowHeight{};
    };

    struct MeshRequest
    {
        entt::entity                   target{entt::null};
        std::string                    name;
        MeshRecord                     record;
        Transform                      transform;
        MeshMaterialData               material;
        std::optional<PhysicsRecord>   physics;
    };

    namespace scene_instantiator
    {
        using Error = std::string;
        std::expected<std::vector<MeshRequest>, Error> apply(
            Scene &scene, const SceneDocument &document, const SceneLoadContext &context
        );
    }
}
