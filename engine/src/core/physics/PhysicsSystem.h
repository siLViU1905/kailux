#pragma once

#include "../scene/Scene.h"
#include "PhysicsRegistry.h"

namespace kailux
{
    class PhysicsSystem
    {
    public:
        PhysicsSystem(Scene& scene, PhysicsRegistry& physicsRegistry);

        void Update(float deltaTime);

        void SetSimulationState(SimulationState state);
        SimulationState GetSimulationState() const;

        void AddPhysicsToEntity(entt::entity entity, PhysicsCreationOptions options);

        void UpdateBodyType(BodyHandle handle, PhysicsBodyType type);
        void UpdateBodyScale(BodyHandle handle, const glm::vec3& scale);

        using OnLog = std::move_only_function<void(std::string_view)>;
        void SetOnWarningLog(OnLog&& callback);

    private:
        void OnSimulationStart();
        void UpdateControls();
        void UpdateTransforms();

        BodyHandle UploadPhysicsBodyDataToRegistry(const PhysicsBodyInfo& data);

        std::reference_wrapper<Scene>           mScene;
        std::reference_wrapper<PhysicsRegistry> mPhysicsRegistry;

        SimulationState mSimulationState{SimulationState::Paused};

        OnLog mOnWarningLog;
    };
}