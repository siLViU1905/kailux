#pragma once
#include <entt/entt.hpp>

#include "Camera.h"
#include "Core.h"
#include "components/entt/GizmoComponent.h"
#include "components/entt/MeshComponent.h"
#include "components/gpu/MeshTransformData.h"
#include "components/gpu/MeshMaterialData.h"
#include "components/gpu/SceneData.h"
#include "mesh/MeshLoader.h"
#include "mesh/MeshRegistry.h"
#include "texture/TextureRegistry.h"

namespace kailux
{
    class Scene
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Scene)

        static Scene create(std::string_view name);

        void update();

        entt::entity createCameraEntity(std::string_view name, bool isPrimary, int width, int height);

        std::optional<entt::entity> createMeshEntity(
            std::string_view name,
            const MeshComponent &component,
            TextureSetHandle textureSetHandle,
            const MeshTransformData &transform,
            const MeshMaterialData &material,
            entt::entity parent = entt::null
        );
        entt::entity createParentEntity(std::string_view name);

        std::optional<entt::entity> createPointLightEntity(std::string_view name, GizmoComponent component, const glm::vec3 &position);

        entt::registry&       getEntityRegistry();
        const entt::registry& getEntityRegistry() const;
        entt::entity          getMainCamera() const;
        void                  setMainCamera(entt::entity camera);
        entt::entity          getSun() const;
        SceneData             getData() const;

        std::string_view      getName() const;

        std::string           getMeshEntityName();
        std::string           getLightEntityName();

        static constexpr std::string_view kSaveFolder = "scenes";
        std::string           serialize() const;
        nlohmann::json        deserialize(std::string_view content, int windowWidth, int windowHeight);

    private:
        static constexpr std::string_view kSunName = "Sun";

        entt::entity createEntity(std::string_view name);
        using        SunData = DirectionalLightData;
        entt::entity createSunEntity(const SunData& data);

        LightsData getLightData() const;

        void updateTransforms();

        std::string    mName{"Scene"};

        entt::registry mEntityRegistry;
        entt::entity   mMainCameraEntity{entt::null};
        entt::entity   mSun{entt::null};

        uint32_t mMeshEntityNameCount{};
        uint32_t mLightEntityNameCount{};
    };
}
