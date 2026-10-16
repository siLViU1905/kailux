#pragma once

#include "PhysicsRegistry.h"

namespace kailux
{
    class Scene;

    class PhysicsSystem
    {
    public:
        explicit PhysicsSystem(PhysicsRegistry& physicsRegistry);

        void Update(Scene& scene, float deltaTime);

        void  SetTickRate(float refreshRate);
        float GetTickRate() const;

        void SetSimulationState(Scene& scene, SimulationState state);
        SimulationState GetSimulationState() const;

        float GetLastUpdateMs() const;

        std::optional<BodyHandle> AddPhysicsToEntity(Scene& scene, entt::entity entity, PhysicsCreationOptions options);

        bool UpdateBodyType(BodyHandle handle, PhysicsBodyType type);
        void UpdateBodyScale(BodyHandle handle, const glm::vec3& scale);

        using OnLog = std::move_only_function<void(std::string_view)>;
        void SetOnInfoLog(OnLog&& callback);
        void SetOnWarningLog(OnLog&& callback);

        void AttachToScene(Scene& scene) const;
        void DestroyAllBodies() const;

    private:
        static constexpr auto kMaxFrameTime{0.25f};


        void OnSimulationStart(Scene& scene);
        void ApplyImpulses(Scene& scene);
        void ApplyForces(Scene& scene);
        void MoveKinematicBodies(Scene& scene, float step);
        void StoreStepResults(Scene& scene) const;
        void ReadVelocities(Scene& scene);
        void UpdateTransforms(Scene& scene, float alpha) const;

        using UploadResult = std::expected<BodyHandle, std::string>;
        UploadResult UploadPhysicsBodyDataToRegistry(const PhysicsBodyInfo& data);

        std::reference_wrapper<PhysicsRegistry> mPhysicsRegistry;

        SimulationState mSimulationState{SimulationState::Paused};

        float mFixedStep{1.f / 60.f};
        float mAccumulator{};
        int   mCollisionSteps{1};

        JPH::EPhysicsUpdateError mLastUpdateError{JPH::EPhysicsUpdateError::None};

        float mLastUpdateMs{};

        OnLog mOnInfoLog;
        OnLog mOnWarningLog;
    };
}