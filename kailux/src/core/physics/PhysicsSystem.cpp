#include "PhysicsSystem.h"

#include "../scene/Scene.h"
#include "../components/entt/CachedPhysicsData.h"
#include "../components/entt/MeshComponent.h"
#include "../components/entt/PhysicsComponent.h"
#include "../components/entt/PhysicsControlComponent.h"
#include "../components/entt/PhysicsInterpolation.h"
#include "core/Clock.h"
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

    void PhysicsSystem::SetOnInfoLog(OnLog &&callback)
    {
        mOnInfoLog = std::move(callback);
    }

    void PhysicsSystem::SetOnWarningLog(OnLog&& callback)
    {
        mOnWarningLog = std::move(callback);
    }

    namespace details
    {
        static void on_physics_component_destroyed(entt::registry& registry, entt::entity entity)
        {
            const auto* physics{registry.ctx().find<PhysicsRegistry*>()};
            if (!physics || !*physics)
                return;

            const auto& component{registry.get<PhysicsComponent>(entity)};
            if (component.handle.Valid())
                (*physics)->DestroyBody(component.handle);
        }
    }

    void PhysicsSystem::AttachToScene() const
    {
        auto& registry{mScene.get().GetEntityRegistry()};
        registry.ctx().insert_or_assign<PhysicsRegistry*>(&mPhysicsRegistry.get());

        auto sink{registry.on_destroy<PhysicsComponent>()};
        sink.disconnect<&details::on_physics_component_destroyed>();
        sink.connect<&details::on_physics_component_destroyed>();
    }

    void PhysicsSystem::DestroyAllBodies() const
    {
        auto& registry{mScene.get().GetEntityRegistry()};
        for (const auto entity : registry.view<PhysicsComponent>())
        {
            auto& component{registry.get<PhysicsComponent>(entity)};
            if (component.handle.Valid())
            {
                mPhysicsRegistry.get().DestroyBody(component.handle);
                component.handle = {};
            }
        }
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

    namespace details
    {
        static std::string describe_update_error(JPH::EPhysicsUpdateError error)
        {
            using E = JPH::EPhysicsUpdateError;

            std::string message{"Physics dropped contacts:"};
            if ((error & E::ManifoldCacheFull) != E::None)
                message += " manifold cache full (raise kMaxContactConstraints);";
            if ((error & E::BodyPairCacheFull) != E::None)
                message += " body pair cache full (raise kMaxBodyPairs);";
            if ((error & E::ContactConstraintsFull) != E::None)
                message += " contact constraints full (raise kMaxContactConstraints);";
            return message;
        }
    }

    void PhysicsSystem::Update(float deltaTime)
    {
        mAccumulator = std::min(mAccumulator + deltaTime, kMaxFrameTime);

        ApplyImpulses();

        while (mAccumulator >= mFixedStep)
        {
            ApplyForces();
            MoveKinematicBodies(mFixedStep);
            const auto updateErr{mPhysicsRegistry.get().Update(mFixedStep, mCollisionSteps)};
            StoreStepResults();
            mAccumulator -= mFixedStep;

            if (updateErr != JPH::EPhysicsUpdateError::None && updateErr != mLastUpdateError)
                mOnWarningLog(details::describe_update_error(updateErr));
            mLastUpdateError = updateErr;
        }

        UpdateTransforms(mAccumulator / mFixedStep);
        ReadVelocities();
    }

    void PhysicsSystem::SetTickRate(float refreshRate)
    {
        mFixedStep = 1.f / std::clamp(refreshRate, 30.f, 240.f);

        mCollisionSteps = static_cast<int>(std::ceil(mFixedStep * 60.f - 1e-4f));
    }

    float PhysicsSystem::GetTickRate() const
    {
        return 1.f / mFixedStep;
    }

    bool PhysicsSystem::UpdateBodyType(BodyHandle handle, PhysicsBodyType type)
    {
        return mPhysicsRegistry.get().SetBodyType(handle, type);
    }

    void PhysicsSystem::UpdateBodyScale(BodyHandle handle, const glm::vec3& scale)
    {
        mPhysicsRegistry.get().UpdateBodyScale(handle, scale);
    }

    PhysicsSystem::UploadResult PhysicsSystem::UploadPhysicsBodyDataToRegistry(const PhysicsBodyInfo& data)
    {
        auto t = Clock::now();
        const auto result{mPhysicsRegistry.get().CreateBody(data)};
        if (result)
            mOnInfoLog(std::format("Physics body attached in {:.3f}ms", Clock::get_elapsed<float, TimeType::Milliseconds>(t)));
        return result;
    }

    void PhysicsSystem::OnSimulationStart()
    {
        mAccumulator = 0.f;
        auto& registry{mScene.get().GetEntityRegistry()};
        const auto view{registry.view<WorldTransform, PhysicsComponent>()};

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
            {
                mPhysicsRegistry.get().ResetVelocities(physicsComp.handle);
                registry.emplace_or_replace<PhysicsInterpolation>(
                    entity,
                    world.position,
                    world.position,
                    world.rotation,
                    world.rotation
                );
            }
        }
    }

    void PhysicsSystem::ApplyImpulses()
    {
        const auto view{mScene.get().GetEntityRegistry().view<PhysicsComponent, PhysicsControlComponent>()};
        view.each([this](const auto& phys, auto& control)
        {
            if (control.applyImpulse && phys.IsDynamic())
                mPhysicsRegistry.get().AddImpulse(phys.handle, control.impulse);
            control.applyImpulse = false;
        });
    }

    void PhysicsSystem::ApplyForces()
    {
        const auto view{mScene.get().GetEntityRegistry().view<PhysicsComponent, PhysicsControlComponent>()};
        view.each([this](const auto& phys, auto& control)
        {
            if (control.applyForce && phys.IsDynamic())
                mPhysicsRegistry.get().AddForce(phys.handle, control.force);
        });
    }

    void PhysicsSystem::MoveKinematicBodies(float step)
    {
        const auto view{mScene.get().GetEntityRegistry().view<WorldTransform, PhysicsComponent>()};
        view.each([this, step](const auto& transform, const auto& phys)
        {
            if (phys.type != PhysicsBodyType::Kinematic)
                return;

            const auto world{Transform::from_matrix(transform.model)};
            mPhysicsRegistry.get().MoveKinematic(phys.handle, world.position, world.rotation, step);
        });
    }

    void PhysicsSystem::StoreStepResults() const
    {
        const auto view{mScene.get().GetEntityRegistry().view<PhysicsComponent, PhysicsInterpolation>()};
        view.each([this](const auto& phys, auto& state)
        {
            if (!phys.IsDynamic())
                return;

            state.prevPosition = state.currPosition;
            state.prevRotation = state.currRotation;
            mPhysicsRegistry.get().GetBodyTransform(phys.handle, state.currPosition, state.currRotation);
        });
    }

    void PhysicsSystem::ReadVelocities()
    {
        const auto view{mScene.get().GetEntityRegistry().view<PhysicsComponent, PhysicsControlComponent>()};
        view.each([this](const auto& phys, auto& control)
        {
            control.velocity = mPhysicsRegistry.get().GetLinearVelocity(phys.handle);
        });
    }

    void PhysicsSystem::UpdateTransforms(float alpha) const
    {
        Scene& scene    = mScene;
        auto& registry{scene.GetEntityRegistry()};
        const auto view{registry.view<LocalTransform, PhysicsComponent>()};

        for (const auto entity : view)
        {
            const auto& physics = view.get<PhysicsComponent>(entity);
            if (!physics.IsDynamic())
                continue;

            glm::vec3 worldPos;
            glm::quat worldRot;
            if (const auto* state{registry.try_get<PhysicsInterpolation>(entity)})
            {
                worldPos = glm::mix(state->prevPosition, state->currPosition, alpha);
                worldRot = glm::slerp(state->prevRotation, state->currRotation, alpha);
            }
            else
                mPhysicsRegistry.get().GetBodyTransform(physics.handle, worldPos, worldRot);

            Transform updated{view.get<LocalTransform>(entity)};

            const auto* hierarchy{registry.try_get<HierarchyComponent>(entity)};
            if (!hierarchy || hierarchy->parent == entt::null)
            {
                updated.position = worldPos;
                updated.rotation = worldRot;
            }
            else
            {
                const auto parent{scene.GetParentWorldMatrix(entity)};

                const auto parentRot{
                    glm::quat_cast(glm::mat3{
                            glm::normalize(glm::vec3{parent[0]}),
                            glm::normalize(glm::vec3{parent[1]}),
                            glm::normalize(glm::vec3{parent[2]})
                        }
                    )
                };

                updated.position = glm::vec3(glm::inverse(parent) * glm::vec4(worldPos, 1.f));
                updated.rotation = glm::normalize(glm::inverse(parentRot) * worldRot);
            }

            scene.SetLocalTransform(entity, updated);
        }
    }

    std::optional<BodyHandle> PhysicsSystem::AddPhysicsToEntity(entt::entity entity, PhysicsCreationOptions options)
    {
        auto& reg{mScene.get().GetEntityRegistry()};

        const auto transform{Transform::from_matrix(reg.get<WorldTransform>(entity).model)};

        PhysicsBodyInfo info{{}, MeshType::Unknown, transform, options};

        if (const auto* cache{reg.try_get<CachedPhysicsData>(entity)})
        {
            info.mesh = cache->mesh;
            info.meshType = cache->meshType;
        }
        else if (const auto* source{reg.try_get<MeshSourceComponent>(entity)})
            info.meshType = source->type;
        else
        {
            mOnWarningLog("Cannot add physics: entity has neither cached physics data nor a mesh component");
            return std::nullopt;
        }

        const auto result{UploadPhysicsBodyDataToRegistry(info)};
        if (!result)
        {
            mOnWarningLog(result.error());
            return std::nullopt;
        }

        if (const auto* old{reg.try_get<PhysicsComponent>(entity)}; old && old->handle.Valid())
        {
            mPhysicsRegistry.get().DestroyBody(old->handle);
            reg.remove<PhysicsInterpolation>(entity);
        }

        return *result;
    }
}
