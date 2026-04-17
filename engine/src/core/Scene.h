#pragma once
#include <entt/entt.hpp>

#include "Camera.h"
#include "Core.h"
#include "components/gpu/MeshTransformData.h"
#include "components/gpu/MeshMaterialData.h"
#include "components/gpu/SceneData.h"
#include "mesh/MeshRegistry.h"
#include "texture/TextureRegistry.h"

namespace kailux
{
    class Scene
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Scene)

        static Scene create();

        entt::entity createCameraEntity(std::string_view name, bool isPrimary, int width, int height);
        entt::entity createMeshEntity(
            std::string_view name,
            MeshHandle meshHandle,
            TextureSetHandle textureSetHandle,
            const MeshTransformData &transform,
            const MeshMaterialData &material
        );

        entt::registry&       getEntityRegistry();
        const entt::registry& getEntityRegistry() const;
        entt::entity          getMainCamera() const;
        void                  setMainCamera(entt::entity camera);
        entt::entity          getSun() const;
        SceneData             getData() const;
        glm::vec4&            getAmbient();
        const glm::vec4&      getAmbient() const;

        std::string           getMeshEntityName();

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

        uint32_t m_MeshEntityNameCount;
    };
}
