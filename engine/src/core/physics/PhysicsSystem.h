#pragma once

#include "../Scene.h"
#include "PhysicsRegistry.h"

namespace kailux
{
    class PhysicsSystem
    {
    public:
        PhysicsSystem(Scene& scene, PhysicsRegistry& physicsRegistry);

        void update(float deltaTime);

        void setSimulationState(SimulationState state);
        SimulationState getSimulationState() const;

        void addPhysicsToEntity(entt::entity entity, PhysicsCreationOptions options);

        void updateBodyType(BodyHandle handle, PhysicsBodyType type);
        void updateBodyScale(BodyHandle handle, const glm::vec3& scale);

        using OnLog = std::move_only_function<void(std::string_view)>;
        void setOnWarningLog(OnLog&& callback);

    private:
        void onSimulationStart();
        void updateControls();
        void updateTransforms();

        BodyHandle uploadPhysicsBodyDataToRegistry(const PhysicsBodyInfo& data);

        std::reference_wrapper<Scene>           mScene;
        std::reference_wrapper<PhysicsRegistry> mPhysicsRegistry;

        SimulationState mSimulationState{SimulationState::Paused};

        OnLog mOnWarningLog;
    };
}