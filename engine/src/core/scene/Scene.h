#pragma once
#include <entt/entt.hpp>

#include "SceneDocument.h"
#include "../Camera.h"
#include "../Core.h"
#include "../components/entt/GizmoComponent.h"
#include "../components/entt/MeshComponent.h"
#include "../components/gpu/MeshTransformData.h"
#include "../components/gpu/MeshMaterialData.h"
#include "../components/gpu/SceneData.h"
#include "../mesh/MeshLoader.h"
#include "../texture/TextureRegistry.h"
#include "core/components/entt/MeshSourceComponent.h"
#include "core/components/entt/PhysicsComponent.h"

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
            MaterialHandle materialHandle,
            const MeshTransformData &transform,
            const MeshMaterialData &material,
            entt::entity parent = entt::null
        );
        entt::entity createParentEntity(std::string_view name);

        std::optional<entt::entity> createPointLightEntity(std::string_view name, const GizmoComponent &component, const glm::vec3 &position);

        entt::registry&       getEntityRegistry();
        const entt::registry& getEntityRegistry() const;
        entt::entity          getMainCamera() const;
        void                  setMainCamera(entt::entity camera);
        entt::entity          getSun() const;
        SceneData             getData() const;

        std::string_view      getName() const;

        std::string           getMeshEntityName();
        std::string           getLightEntityName();

        void                         setSavePath(const std::filesystem::path& path);
        const std::filesystem::path& getSavePath() const;

        void      setMeta(const SceneMeta& meta);
        SceneMeta getMeta() const;

        bool attachMesh(entt::entity entity,
                        const MeshComponent          &component,
                        MaterialHandle               materialHandle,
                        const MeshMaterialData       &material);
        bool attachMeshSource(entt::entity entity, const MeshSourceComponent& source);
        bool attachPointLight(entt::entity            entity,
                              const GizmoComponent   &component,
                              const PointLightRecord &light);
        void attachPhysics(entt::entity entity, PhysicsComponent component);
        void attachCamera(entt::entity entity, const CameraComponent& component, int width, int height);

        void setLocalTransform(entt::entity entity, const MeshTransformData &transform);
        void setParent(entt::entity child, entt::entity parent);
        void detachFromParent(entt::entity child);
        void destroyEntity(entt::entity entity);

        friend class SceneInstantiator;

    private:
        static constexpr std::string_view kSunName = "Sun";

        entt::entity createEntity(std::string_view name);
        using        SunData = DirectionalLightData;
        entt::entity createSunEntity(const SunData& data);

        LightsData getLightData() const;

        void updateTransforms();

        std::string    mName{"Scene"};

        std::filesystem::path mSavePath{};

        entt::registry mEntityRegistry;
        entt::entity   mMainCameraEntity{entt::null};
        entt::entity   mSun{entt::null};

        uint32_t mMeshEntityNameCount{};
        uint32_t mLightEntityNameCount{};
    };
}
