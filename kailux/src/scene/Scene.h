#pragma once
#include <entt/entt.hpp>

#include "SceneDocument.h"
#include "../Camera.h"
#include "../Core.h"
#include "../components/entt/GizmoComponent.h"
#include "../components/entt/MeshComponent.h"
#include "../components/gpu/MeshMaterialData.h"
#include "../components/gpu/SceneData.h"
#include "../mesh/MeshLoader.h"
#include "../texture/TextureRegistry.h"
#include "components/entt/MeshSourceComponent.h"
#include "components/entt/PhysicsComponent.h"
#include "components/math/Transform.h"

namespace kailux
{
    using SceneId = uint32_t;
    inline constexpr SceneId kInvalidSceneId{};

    class Scene
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Scene)

        static Scene create(SceneId id, std::string_view name);

        void Update();

        void UpdateCameras(entt::entity controlled = entt::null);

        void SyncCameraTransform(entt::entity entity);

        template<typename Type, typename... Other, typename... Exclude>
        uint32_t GetEntityCount(entt::exclude_t<Exclude...> exclude = entt::exclude_t{}) const
        {
            const auto view{mEntityRegistry.view<Type, Other...>(exclude)};

            if constexpr (sizeof...(Other) == 0 && sizeof...(Exclude) == 0)
                return static_cast<uint32_t>(view.size());
            else
            {
                uint32_t count{};
                for ([[maybe_unused]] const auto e : view)
                    ++count;
                return count;
            }
        }

        entt::entity CreateEntity(std::string_view name);

        using CreateResult = std::expected<entt::entity, std::string>;

        CreateResult CreateCameraEntity(std::string_view name, const GizmoComponent &component, const glm::vec3 &position, bool isPrimary);
        entt::entity CreateBuiltinCameraEntity(std::string_view name);

        CreateResult CreateMeshEntity(
            std::string_view name,
            const MeshComponent &component,
            MaterialHandle materialHandle,
            const Transform &transform,
            const MeshMaterialData &material,
            entt::entity parent = entt::null
        );
        entt::entity CreateParentEntity(std::string_view name);

        CreateResult CreatePointLightEntity(
            std::string_view name,
            const GizmoComponent &component,
            const glm::vec3 &position);
        std::array<entt::entity, details::kMaxPointLights> GetPointLightEntities() const;

        entt::registry&       GetEntityRegistry();
        const entt::registry& GetEntityRegistry() const;
        entt::entity          GetSceneCamera() const;
        void                  SetPrimaryCamera(entt::entity entity);
        entt::entity          GetPrimaryCamera() const;
        entt::entity          GetNextCamera(entt::entity currentCamera) const;
        void                  SetMainCamera(entt::entity camera);
        entt::entity          GetSun() const;
        entt::entity          GetSettingsEntity() const;
        SceneData             GetData() const;
        LightsData            GetLightData() const;

        std::string_view      GetName() const;
        SceneId               GetId() const;

        std::string           GetMeshEntityName();
        std::string           GetLightEntityName();
        std::string           GetCameraEntityName();

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
        void AttachCamera(entt::entity entity, const GizmoComponent &component, const CameraComponent &camera);
        void AttachBuiltinCamera(entt::entity entity);

        glm::mat4 GetParentWorldMatrix(entt::entity entity) const;
        void      SetLocalTransform(entt::entity entity, const Transform &transform);
        void      SetWorldTransform(entt::entity entity, const glm::mat4 &world);

        void SetParent(entt::entity child, entt::entity parent);
        void DetachFromParent(entt::entity child);

        void DestroyEntity(entt::entity entity);

        friend class SceneInstantiator;

    private:
        static constexpr std::string_view kSunName = "Sun";

        using        SunData = DirectionalLightData;
        entt::entity CreateSunEntity(const SunData& data);
        void         CreateSceneCamera();
        void         CreateSceneSettings();

        void UpdateTransforms();

        SceneId        mId{kInvalidSceneId};
        std::string    mName{"Scene"};

        std::filesystem::path mSavePath{};

        entt::registry mEntityRegistry;
        entt::entity   mSceneCameraEntity{entt::null};
        entt::entity   mSun{entt::null};
        entt::entity   mSettingsEntity{entt::null};

        uint32_t mMeshEntityNameCount{};
        uint32_t mLightEntityNameCount{};
        uint32_t mCameraEntityNameCount{};
    };
}