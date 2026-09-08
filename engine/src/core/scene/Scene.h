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

        using CreateResult = std::expected<entt::entity, std::string>;

        CreateResult CreateCameraEntity(std::string_view name, const GizmoComponent &component, const glm::vec3 &position, bool isPrimary);
        entt::entity CreateBuiltinCameraEntity(std::string_view name);

        CreateResult CreateMeshEntity(
            std::string_view name,
            const MeshComponent &component,
            MaterialHandle materialHandle,
            const MeshTransformData &transform,
            const MeshMaterialData &material,
            entt::entity parent = entt::null
        );
        entt::entity CreateParentEntity(std::string_view name);

        CreateResult CreatePointLightEntity(std::string_view name, const GizmoComponent &component,
                                            const glm::vec3 &position);

        entt::registry&       GetEntityRegistry();
        const entt::registry& GetEntityRegistry() const;
        entt::entity          GetSceneCamera() const;
        void                  SetPrimaryCamera(entt::entity entity);
        entt::entity          GetPrimaryCamera() const;
        entt::entity          GetNextCamera(entt::entity currentCamera) const;
        void                  SetMainCamera(entt::entity camera);
        entt::entity          GetSun() const;
        SceneData             GetData() const;

        std::string_view      GetName() const;

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
        void         CreateSceneCamera();

        LightsData GetLightData() const;

        void UpdateTransforms();

        std::string    mName{"Scene"};

        std::filesystem::path mSavePath{};

        entt::registry mEntityRegistry;
        entt::entity   mSceneCameraEntity{entt::null};
        entt::entity   mSun{entt::null};

        uint32_t mMeshEntityNameCount{};
        uint32_t mLightEntityNameCount{};
        uint32_t mCameraEntityNameCount{};
    };
}