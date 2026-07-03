#pragma once

#include "impl/BroadPhaseLayer.h"
#include "impl/ObjectVsBroadPhaseLayerFilter.h"
#include "impl/ObjectLayerPairFilter.h"
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>

#include "core/Core.h"
#include "core/components/gpu/MeshTransformData.h"
#include "core/mesh/MeshRegistry.h"

namespace kailux
{
    using BodyHandle = Handle;

    enum class PhysicsBodyType : uint8_t
    {
        Static = static_cast<uint8_t>(JPH::EMotionType::Static),
        Kinematic = static_cast<uint8_t>(JPH::EMotionType::Kinematic),
        Dynamic = static_cast<uint8_t>(JPH::EMotionType::Dynamic)
    };

    struct SubmeshPhysicsInfo
    {
        std::vector<Vertex>                  vertices;
        std::vector<MeshRegistry::IndexType> indices;
        glm::mat4                            localTransform{1.f};
    };

    struct PhysicsCreationOptions
    {
        PhysicsBodyType bodyType{PhysicsBodyType::Static};
        bool            canBecomeDynamic{true};
    };

    struct PhysicsBodyInfo
    {
        std::vector<SubmeshPhysicsInfo> submeshes;
        MeshType                        meshType;
        MeshTransformData               transform;
        PhysicsCreationOptions          options;
    };

    class PhysicsRegistry
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(PhysicsRegistry)

        static PhysicsRegistry create();

        BodyHandle createBody(const PhysicsBodyInfo& info);
        void       destroyBody(BodyHandle handle);
        void       setBodyEnabled(BodyHandle handle, bool enabled = true);
        bool       isBodyEnabled(BodyHandle handle) const;
        void       setBodyType(BodyHandle handle, PhysicsBodyType type);

        void      addForce(BodyHandle handle, const glm::vec3& force);
        void      addImpulse(BodyHandle handle, const glm::vec3& impulse);
        void      setLinearVelocity(BodyHandle handle, const glm::vec3& velocity);
        glm::vec3 getLinearVelocity(BodyHandle handle) const;

        void setBodyTransform(BodyHandle handle, const glm::vec3& position, const glm::quat& rotation);
        void getBodyTransform(BodyHandle handle, glm::vec3& outPosition, glm::quat& outRotation) const;

        void updateBodyScale(BodyHandle handle, const glm::vec3& scale);

        void update(float deltaTime);

    private:
        static constexpr uint32_t kAllocatorSize = 10 * 1024 * 1024;

        static constexpr uint32_t kMaxBodies = 65536;
        static constexpr uint32_t kNumBodyMutexes = 0;
        static constexpr uint32_t kMaxBodyPairs = 65536;
        static constexpr uint32_t kMaxContactConstraints = 10240;

        static constexpr uint32_t kCollisionSteps = 1;

        static uint32_t pick_thread_count(uint32_t freeThreads);

        void allocResources();

        uint32_t acquireSlot();

        static JPH::ShapeRefC create_builtin_mesh_body(MeshType type, const MeshTransformData &transform);
        static JPH::ShapeRefC create_loaded_mesh_body(const PhysicsBodyInfo &info);

        Scoped<JPH::TempAllocatorImpl>              m_Allocator;
        Scoped<JPH::JobSystemThreadPool>            m_JobSystem;
        Scoped<impl::BroadPhaseLayer>               m_BroadPhaseLayer;
        Scoped<impl::ObjectVsBroadPhaseLayerFilter> m_ObjectVsBroadPhaseLayer;
        Scoped<impl::ObjectLayerPairFilter>         m_ObjectPairFilter;
        Scoped<JPH::PhysicsSystem>                  m_PhysicsSystem;

        std::vector<JPH::BodyID> m_BodyIds;
        std::deque<uint32_t>     m_FreeSlots;
    };
}
