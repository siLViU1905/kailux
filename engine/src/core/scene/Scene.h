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

        static Scene create(std::string_view name, const Window &window);

        void Update();

        std::optional<entt::entity> CreateCameraEntity(std::string_view name, bool isPrimary, int width, int height);

        std::optional<entt::entity> CreateMeshEntity(
            std::string_view name,
            const MeshComponent &component,
            MaterialHandle materialHandle,
            const MeshTransformData &transform,
            const MeshMaterialData &material,
            entt::entity parent = entt::null
        );
        entt::entity CreateParentEntity(std::string_view name);

        std::optional<entt::entity> CreatePointLightEntity(std::string_view name, const GizmoComponent &component, const glm::vec3 &position);

        entt::registry&       GetEntityRegistry();
        const entt::registry& GetEntityRegistry() const;
        entt::entity          GetSceneCamera() const;
        entt::entity          GetSimulationCamera() const;
        void                  SetMainCamera(entt::entity camera);
        entt::entity          GetSun() const;
        SceneData             GetData() const;

        std::string_view      GetName() const;

        std::string           GetMeshEntityName();
        std::string           GetLightEntityName();

        void                         SetSavePath(const std::filesystem::path& path);
        const std::filesystem::path& GetSavePath() const;

        void      SetMeta(const SceneMeta& meta);
        SceneMeta GetMeta() const;

        bool AttachMesh(entt::entity entity,
                        const MeshComponent          &component,
                        MaterialHandle               materialHandle,
                        const MeshMaterialData       &material);
        bool AttachMeshSource(entt::entity entity, const MeshSourceComponent& source);
        bool AttachPointLight(entt::entity            entity,
                              const GizmoComponent   &component,
                              const PointLightRecord &light);
        void AttachPhysics(entt::entity entity, PhysicsComponent component);
        void AttachCamera(entt::entity entity, const CameraComponent &component);

        void SetLocalTransform(entt::entity entity, const MeshTransformData &transform);
        void SetParent(entt::entity child, entt::entity parent);
        void DetachFromParent(entt::entity child);
        void DestroyEntity(entt::entity entity);

        friend class SceneInstantiator;

    private:
        static constexpr std::string_view kSunName = "Sun";

        entt::entity CreateEntity(std::string_view name);
        using        SunData = DirectionalLightData;
        entt::entity CreateSunEntity(const SunData& data);
        void         CreateCameras(const Window& window);

        LightsData GetLightData() const;

        void UpdateTransforms();

        std::string    mName{"Scene"};

        std::filesystem::path mSavePath{};

        entt::registry mEntityRegistry;
        entt::entity   mSceneCameraEntity{entt::null};
        entt::entity   mSimulationCameraEntity{entt::null};
        entt::entity   mSun{entt::null};

        uint32_t mMeshEntityNameCount{};
        uint32_t mLightEntityNameCount{};
    };
}
