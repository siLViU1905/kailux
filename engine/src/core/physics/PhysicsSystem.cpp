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
        auto view = registry.view<TransformComponent, PhysicsComponent>();

        for (auto entity : view)
        {
            const auto& transformComp = view.get<TransformComponent>(entity);
            const auto& physicsComp = view.get<PhysicsComponent>(entity);

            mPhysicsRegistry.get().SetBodyTransform(
                physicsComp.handle,
                transformComp.transform.position,
                transformComp.transform.rotation
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
        auto& registry = mScene.get().GetEntityRegistry();
        auto view = registry.view<TransformComponent, PhysicsComponent>();

        for (auto entity : view)
        {
            auto& transformComp = view.get<TransformComponent>(entity);
            auto physics = view.get<PhysicsComponent>(entity);

            if (physics.IsDynamic())
            {
                glm::vec3 worldPos;
                glm::quat worldRot;
                mPhysicsRegistry.get().GetBodyTransform(physics.handle, worldPos, worldRot);

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

    void PhysicsSystem::AddPhysicsToEntity(entt::entity entity, PhysicsCreationOptions options)
    {
        auto& reg = mScene.get().GetEntityRegistry();

        const auto& transform = reg.get<TransformComponent>(entity).transform;

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
