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

        static Scene create(std::string_view name);

        entt::entity createCameraEntity(std::string_view name, bool isPrimary, int width, int height);
        entt::entity createMeshEntity(
            std::string_view name,
            MeshHandle meshHandle,
            std::string_view path,
            TextureSetHandle textureSetHandle,
            const MeshTransformData &transform, const MeshMaterialData &material
        );

        entt::registry&       getEntityRegistry();
        const entt::registry& getEntityRegistry() const;
        entt::entity          getMainCamera() const;
        void                  setMainCamera(entt::entity camera);
        entt::entity          getSun() const;
        SceneData             getData() const;
        glm::vec4&            getAmbient();
        const glm::vec4&      getAmbient() const;
        std::string_view      getName() const;

        std::string           getMeshEntityName();

        static constexpr std::string_view s_SaveFolder = "scenes";
        std::string           serialize() const;

    private:
        static constexpr std::string_view s_SunName = "Sun";

        entt::entity createEntity(std::string_view name);
        using        SunData = DirectionalLightData;
        entt::entity createSunEntity(const SunData& data);

        std::string    m_Name;

        entt::registry m_EntityRegistry;
        entt::entity   m_MainCameraEntity;
        entt::entity   m_Sun;

        glm::vec4      m_Ambient;

        uint32_t m_MeshEntityNameCount;
    };
}
