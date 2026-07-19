#include "PhysicsSystem.h"

#include "../components/entt/CachedPhysicsData.h"
#include "../components/entt/MeshComponent.h"
#include "../components/entt/PhysicsComponent.h"
#include "../components/entt/PhysicsControlComponent.h"
#include "../components/gpu/TransformComponent.h"
#include "core/components/entt/HierarchyComponent.h"

namespace kailux
{
    PhysicsSystem::PhysicsSystem(Scene& scene, PhysicsRegistry& physicsRegistry)
        : mScene(scene)
        , mPhysicsRegistry(physicsRegistry)
    {
    }

    void PhysicsSystem::setOnWarningLog(OnLog&& callback)
    {
        mOnWarningLog = std::move(callback);
    }

    SimulationState PhysicsSystem::getSimulationState() const
    {
        return mSimulationState;
    }

    void PhysicsSystem::setSimulationState(SimulationState state)
    {
        mSimulationState = state;
        if (mSimulationState == SimulationState::Running)
            onSimulationStart();
    }

    void PhysicsSystem::update(float deltaTime)
    {
        updateControls();
        updateTransforms();
        mPhysicsRegistry.get().update(deltaTime);
    }

    void PhysicsSystem::updateBodyType(BodyHandle handle, PhysicsBodyType type)
    {
        mPhysicsRegistry.get().setBodyType(handle, type);
    }

    void PhysicsSystem::updateBodyScale(BodyHandle handle, const glm::vec3& scale)
    {
        mPhysicsRegistry.get().updateBodyScale(handle, scale);
    }

    BodyHandle PhysicsSystem::uploadPhysicsBodyDataToRegistry(const PhysicsBodyInfo& data)
    {
        return mPhysicsRegistry.get().createBody(data);
    }

    void PhysicsSystem::onSimulationStart()
    {
        auto& registry = mScene.get().getEntityRegistry();
        auto view = registry.view<TransformComponent, PhysicsComponent>();

        for (auto entity : view)
        {
            const auto& transformComp = view.get<TransformComponent>(entity);
            const auto& physicsComp = view.get<PhysicsComponent>(entity);

            mPhysicsRegistry.get().setBodyTransform(
                physicsComp.handle,
                transformComp.transform.position,
                transformComp.transform.rotation
            );

            if (physicsComp.isDynamic())
                mPhysicsRegistry.get().setLinearVelocity(physicsComp.handle, glm::vec3(0.f));
        }
    }

    void PhysicsSystem::updateControls()
    {
        auto& registry = mScene.get().getEntityRegistry();
        auto view = registry.view<PhysicsComponent, PhysicsControlComponent>();

        for (auto entity : view)
        {
            auto phys = view.get<PhysicsComponent>(entity);
            auto& control = view.get<PhysicsControlComponent>(entity);

            control.velocity = mPhysicsRegistry.get().getLinearVelocity(phys.handle);

            if (control.applyForce)
                mPhysicsRegistry.get().addForce(phys.handle, control.force);

            if (control.applyImpulse)
                mPhysicsRegistry.get().addImpulse(phys.handle, control.impulse);
        }
    }

    void PhysicsSystem::updateTransforms()
    {
        auto& registry = mScene.get().getEntityRegistry();
        auto view = registry.view<TransformComponent, PhysicsComponent>();

        for (auto entity : view)
        {
            auto& transformComp = view.get<TransformComponent>(entity);
            auto physics = view.get<PhysicsComponent>(entity);

            if (physics.isDynamic())
            {
                glm::vec3 worldPos;
                glm::quat worldRot;
                mPhysicsRegistry.get().getBodyTransform(physics.handle, worldPos, worldRot);

                if (auto* h = registry.try_get<HierarchyComponent>(entity);
                    h && h->parent != entt::null)
                {
                    const auto& parentWorld = registry.get<TransformComponent>(h->parent).worldMatrix;
                    glm::mat4 worldM = glm::translate(glm::mat4(1.f), worldPos) * glm::toMat4(worldRot);
                    glm::mat4 localM = glm::inverse(parentWorld) * worldM;

                    transformComp.transform.position = glm::vec3(localM[3]);
                    transformComp.transform.rotation = glm::quat_cast(localM);
                }
                else
                {
                    transformComp.transform.position = worldPos;
                    transformComp.transform.rotation = worldRot;
                }
            }
        }
    }

    void PhysicsSystem::addPhysicsToEntity(entt::entity entity, PhysicsCreationOptions options)
    {
        auto& reg = mScene.get().getEntityRegistry();

        const auto& transform = reg.get<TransformComponent>(entity).transform;

        BodyHandle handle;
        if (const auto* cache = reg.try_get<CachedPhysicsData>(entity))
        {
            std::vector<SubmeshPhysicsInfo> infos;
            infos.reserve(cache->submeshes.size());
            for (const auto& sm : cache->submeshes)
                infos.emplace_back(sm.vertices, sm.indices, sm.localTransform);

            handle = uploadPhysicsBodyDataToRegistry({
                std::move(infos),
                cache->meshType,
                transform,
                {
                    options.bodyType,
                    options.canBecomeDynamic
                }
            });
        }
        else if (const auto* mesh = reg.try_get<MeshComponent>(entity))
        {
            handle = uploadPhysicsBodyDataToRegistry({
                {},
                mesh->type,
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
