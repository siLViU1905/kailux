#pragma once
#include <entt/entt.hpp>

#include "Camera.h"
#include "Core.h"
#include "components/gpu/MeshTransformData.h"
#include "mesh/MeshRegistry.h"

namespace kailux
{
    class Scene
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Scene)

        static Scene create();

        entt::entity createCameraEntity(std::string_view name, const Camera& camera, bool isPrimary);
        entt::entity createMeshEntity(std::string_view name, MeshHandle handle, const MeshTransformData& transform);

        entt::registry&       getEntityRegistry();
        const entt::registry& getEntityRegistry() const;
        entt::entity          getMainCamera() const;
        void                  setMainCamera(entt::entity camera);

    private:
        entt::entity createEntity(std::string_view name);

        entt::registry m_EntityRegistry;
        entt::entity   m_MainCameraEntity;
    };
}
