#pragma once

#include "PhysicsRegistry.h"

namespace kailux
{
    class Scene;

    class PhysicsSystem
    {
    public:
        PhysicsSystem(Scene& scene, PhysicsRegistry& physicsRegistry);

        void Update(float deltaTime);

        void SetSimulationState(SimulationState state);
        SimulationState GetSimulationState() const;

        std::optional<BodyHandle> AddPhysicsToEntity(entt::entity entity, PhysicsCreationOptions options);

        void UpdateBodyType(BodyHandle handle, PhysicsBodyType type);
        void UpdateBodyScale(BodyHandle handle, const glm::vec3& scale);

        using OnLog = std::move_only_function<void(std::string_view)>;
        void SetOnInfoLog(OnLog&& callback);
        void SetOnWarningLog(OnLog&& callback);

        void AttachToScene() const;
        void DestroyAllBodies() const;

    private:
        void OnSimulationStart();
        void UpdateControls();
        void UpdateTransforms();

        using UploadResult = std::expected<BodyHandle, std::string>;
        UploadResult UploadPhysicsBodyDataToRegistry(const PhysicsBodyInfo& data);

        std::reference_wrapper<Scene>           mScene;
        std::reference_wrapper<PhysicsRegistry> mPhysicsRegistry;

        SimulationState mSimulationState{SimulationState::Paused};

        OnLog mOnInfoLog;
        OnLog mOnWarningLog;
    };
}