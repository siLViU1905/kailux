#include "PhysicsSystem.h"

#include "../components/entt/CachedPhysicsData.h"
#include "../components/entt/MeshComponent.h"
#include "../components/entt/PhysicsComponent.h"
#include "../components/entt/PhysicsControlComponent.h"
#include "core/components/entt/HierarchyComponent.h"
#include "core/components/entt/LocalTransform.h"
#include "core/components/entt/WorldTransform.h"

namespace kailux
{
    PhysicsSystem::PhysicsSystem(Scene& scene, PhysicsRegistry& physicsRegistry)
        : mScene(scene)
        , mPhysicsRegistry(physicsRegistry)
    {
    }

    void PhysicsSystem::SetOnWarningLog(OnLog&& callback)
    {
        mOnWarningLog = std::move(callback);
    }

    SimulationState PhysicsSystem::GetSimulationState() const
    {
        return mSimulationState;
    }

    void PhysicsSystem::SetSimulationState(SimulationState state)
    {
        mSimulationState = state;
        if (mSimulationState == SimulationState::Running)
            OnSimulationStart();
    }

    void PhysicsSystem::Update(float deltaTime)
    {
        UpdateControls();
        UpdateTransforms();
        mPhysicsRegistry.get().Update(deltaTime);
    }

    void PhysicsSystem::UpdateBodyType(BodyHandle handle, PhysicsBodyType type)
    {
        mPhysicsRegistry.get().SetBodyType(handle, type);
    }

    void PhysicsSystem::UpdateBodyScale(BodyHandle handle, const glm::vec3& scale)
    {
        mPhysicsRegistry.get().UpdateBodyScale(handle, scale);
    }

    BodyHandle PhysicsSystem::UploadPhysicsBodyDataToRegistry(const PhysicsBodyInfo& data)
    {
        return mPhysicsRegistry.get().CreateBody(data);
    }

    void PhysicsSystem::OnSimulationStart()
    {
        auto& registry = mScene.get().GetEntityRegistry();
        const auto view = registry.view<WorldTransform, PhysicsComponent>();

        for (const auto entity : view)
        {
            const auto world{Transform::from_matrix(view.get<WorldTransform>(entity).model)};
            const auto& physicsComp = view.get<PhysicsComponent>(entity);

            mPhysicsRegistry.get().SetBodyTransform(
                physicsComp.handle,
                world.position,
                world.rotation
            );

            if (physicsComp.IsDynamic())
                mPhysicsRegistry.get().SetLinearVelocity(physicsComp.handle, glm::vec3(0.f));
        }
    }

    void PhysicsSystem::UpdateControls()
    {
        auto& registry = mScene.get().GetEntityRegistry();
        auto view = registry.view<PhysicsComponent, PhysicsControlComponent>();

        for (auto entity : view)
        {
            auto phys = view.get<PhysicsComponent>(entity);
            auto& control = view.get<PhysicsControlComponent>(entity);

            control.velocity = mPhysicsRegistry.get().GetLinearVelocity(phys.handle);

            if (control.applyForce)
                mPhysicsRegistry.get().AddForce(phys.handle, control.force);

            if (control.applyImpulse)
                mPhysicsRegistry.get().AddImpulse(phys.handle, control.impulse);
        }
    }

    void PhysicsSystem::UpdateTransforms()
    {
        Scene& scene = mScene;
        auto &registry = scene.GetEntityRegistry();
        const auto view = registry.view<LocalTransform, PhysicsComponent>();

        for (const auto entity: view)
        {
            const auto &physics = view.get<PhysicsComponent>(entity);
            if (!physics.IsDynamic())
                continue;

            glm::vec3 worldPos;
            glm::quat worldRot;
            mPhysicsRegistry.get().GetBodyTransform(physics.handle, worldPos, worldRot);

            const auto local = glm::inverse(scene.GetParentWorldMatrix(entity))
                                    * glm::translate(glm::mat4(1.f), worldPos)
                                    * glm::toMat4(worldRot);

            Transform updated{view.get<LocalTransform>(entity)};
            updated.position = glm::vec3(local[3]);
            updated.rotation = glm::quat_cast(local);
            scene.SetLocalTransform(entity, updated);
        }
    }

    void PhysicsSystem::AddPhysicsToEntity(entt::entity entity, PhysicsCreationOptions options)
    {
        auto& reg = mScene.get().GetEntityRegistry();

        const auto transform{Transform::from_matrix(reg.get<WorldTransform>(entity).model)};

        BodyHandle handle;
        if (const auto* cache = reg.try_get<CachedPhysicsData>(entity))
        {
            std::vector<SubmeshPhysicsInfo> infos;
            infos.reserve(cache->submeshes.size());
            for (const auto& sm : cache->submeshes)
                infos.emplace_back(sm.vertices, sm.indices, sm.localTransform);

            handle = UploadPhysicsBodyDataToRegistry({
                std::move(infos),
                cache->meshType,
                transform,
                {
                    options.bodyType,
                    options.canBecomeDynamic
                }
            });
        }
        else if (const auto* source = reg.try_get<MeshSourceComponent>(entity))
        {
            handle = UploadPhysicsBodyDataToRegistry({
                {},
                source->type,
                transform,
                {
                    options.bodyType,
                    options.canBecomeDynamic
                }
            });
        }
        else
        {
            mOnWarningLog("Cannot add physics: entity has neither cached physics data nor a mesh component");
            return;
        }

        reg.emplace<PhysicsComponent>(entity, handle, options.bodyType);
        reg.emplace<PhysicsControlComponent>(entity);
    }
}
