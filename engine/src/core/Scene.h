#pragma once
#include <entt/entt.hpp>

#include "Camera.h"
#include "Core.h"
#include "components/gpu/MeshTransformData.h"
#include "components/gpu/MeshMaterialData.h"
#include "components/gpu/SceneData.h"
#include "mesh/MeshRegistry.h"

namespace kailux
{
    class Scene
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Scene)

        static Scene create();

        entt::entity createCameraEntity(std::string_view name, const Camera& camera, bool isPrimary);
        entt::entity createMeshEntity(std::string_view name, MeshHandle handle, const MeshTransformData& transform, const MeshMaterialData & material);

        entt::registry&       getEntityRegistry();
        const entt::registry& getEntityRegistry() const;
        entt::entity          getMainCamera() const;
        void                  setMainCamera(entt::entity camera);
        entt::entity          getSun() const;
        SceneData             getData() const;
        glm::vec4&            getAmbient();
        const glm::vec4&      getAmbient() const;

    private:
        static constexpr std::string_view s_SunName = "Sun";
        static constexpr std::string_view s_SceneName = "Scene";

        entt::entity createEntity(std::string_view name);
        using        SunData = DirectionalLightData;
        entt::entity createSunEntity(const SunData& data);

        entt::registry m_EntityRegistry;
        entt::entity   m_MainCameraEntity;
        entt::entity   m_Sun;

        glm::vec4      m_Ambient;
    };
}
