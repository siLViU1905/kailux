#pragma once
#include <expected>

#include "SceneDocument.h"
#include "core/gizmo/GizmoRegistry.h"
#include "core/physics/PhysicsRegistry.h"

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
        MeshTransformData              transform;
        MeshMaterialData               material;
        std::optional<PhysicsRecord>   physics;
    };

    class SceneInstantiator
    {
    public:
        using Error = std::string;

        static std::expected<std::vector<MeshRequest>, Error> apply(
            Scene &scene, const SceneDocument &document, const SceneLoadContext &context);
    };

}
