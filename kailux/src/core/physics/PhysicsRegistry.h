#pragma once

#include <expected>

#include "impl/BroadPhaseLayer.h"
#include "impl/ObjectVsBroadPhaseLayerFilter.h"
#include "impl/ObjectLayerPairFilter.h"
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>

#include "core/Core.h"
#include "core/components/math/Transform.h"
#include "core/mesh/MeshRegistry.h"

namespace kailux
{
    using BodyHandle = Handle;

    enum class PhysicsBodyType : uint8_t
    {
        Static = static_cast<uint8_t>(JPH::EMotionType::Static),
        Kinematic = static_cast<uint8_t>(JPH::EMotionType::Kinematic),
        Dynamic = static_cast<uint8_t>(JPH::EMotionType::Dynamic),
        Unknown
    };

    struct SubmeshPhysicsInfo
    {
        std::vector<glm::vec3>               positions;
        std::vector<MeshRegistry::IndexType> indices;
        glm::mat4                            localTransform{1.f};
    };

    struct PhysicsMeshData
    {
        std::string                     key;
        std::vector<SubmeshPhysicsInfo> submeshes;
    };

    struct PhysicsCreationOptions
    {
        PhysicsBodyType bodyType{PhysicsBodyType::Static};
        bool            canBecomeDynamic{true};
        float           mass{};
    };

    struct PhysicsBodyInfo
    {
        Shared<const PhysicsMeshData> mesh;
        MeshType                      meshType{MeshType::Unknown};
        Transform                     transform;
        PhysicsCreationOptions        options;
    };

    class PhysicsRegistry
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(PhysicsRegistry)

        static PhysicsRegistry create();

        using BodyResult = std::expected<BodyHandle, std::string>;
        BodyResult CreateBody(const PhysicsBodyInfo& info);
        void       DestroyBody(BodyHandle handle);
        void       SetBodyEnabled(BodyHandle handle, bool enabled = true);
        bool       IsBodyEnabled(BodyHandle handle) const;

        bool SetBodyType(BodyHandle handle, PhysicsBodyType type);

        void      AddForce(BodyHandle handle, const glm::vec3& force);
        void      AddImpulse(BodyHandle handle, const glm::vec3& impulse);
        void      SetLinearVelocity(BodyHandle handle, const glm::vec3& velocity);
        glm::vec3 GetLinearVelocity(BodyHandle handle) const;

        void ResetVelocities(BodyHandle handle);

        void SetBodyTransform(BodyHandle handle, const glm::vec3& position, const glm::quat& rotation);
        void GetBodyTransform(BodyHandle handle, glm::vec3& outPosition, glm::quat& outRotation) const;

        void UpdateBodyScale(BodyHandle handle, const glm::vec3& scale);

        void SetBodyMass(BodyHandle handle, float mass);

        JPH::EPhysicsUpdateError Update(float deltaTime, int collisionSteps);

        void MoveKinematic(
            BodyHandle handle,
            const glm::vec3 &position,
            const glm::quat &rotation,
            float deltaTime
        );

        void PruneShapeCache();

        void OptimizeBroadPhaseIfNeeded();

    private:
        static constexpr uint32_t kAllocatorSize = 16 * 1024 * 1024;

        static constexpr uint32_t kMaxBodies = 65536;
        static constexpr uint32_t kNumBodyMutexes = 0;
        static constexpr uint32_t kMaxBodyPairs = 65536;
        static constexpr uint32_t kMaxContactConstraints = 65536;

        static uint32_t pick_thread_count(uint32_t freeThreads);

        static bool is_valid_pose(const glm::vec3& position, const glm::quat& rotation);
        static bool is_degenerate_triangle(const JPH::Float3 &a, const JPH::Float3 &b, const JPH::Float3 &c);
        static bool scale_is_usable(const glm::vec3& scale);

        static JPH::ShapeRefC make_scaled_shape(const JPH::ShapeRefC& base, const glm::vec3& scale);

        void ApplyMassOverride(JPH::BodyID id, float mass);

        void AllocResources();

        uint32_t AcquireSlot();

        using CreateResult = std::expected<JPH::ShapeRefC, std::string>;
        CreateResult GetOrCreateLoadedShape(const PhysicsBodyInfo& info);

        static CreateResult create_builtin_mesh_body(MeshType type);
        static CreateResult create_loaded_mesh_body(const PhysicsBodyInfo &info);

        decltype(auto) GetBodies(this auto&& self)
        {
            assert(std::this_thread::get_id() == self.mOwnerThreadId && "PhysicsRegistry used off the main thread");
            return (self.mPhysicsSystem->GetBodyInterfaceNoLock());
        }
        const JPH::BodyLockInterface& GetLocks() const;

        struct ChildShapeResult
        {
            JPH::ShapeRefC shape;
            JPH::Vec3      trans{JPH::Vec3::sZero()};
            JPH::Quat      rot{JPH::Quat::sIdentity()};
        };

        using BuildResult = std::expected<void, std::string>;
        static BuildResult build_submesh_shape(const PhysicsBodyInfo &info, size_t idx, ChildShapeResult &out);

        Scoped<JPH::TempAllocatorImplWithMallocFallback> mAllocator;
        Scoped<JPH::JobSystemThreadPool>                 mJobSystem;
        Scoped<impl::BroadPhaseLayer>                    mBroadPhaseLayer;
        Scoped<impl::ObjectVsBroadPhaseLayerFilter>      mObjectVsBroadPhaseLayer;
        Scoped<impl::ObjectLayerPairFilter>              mObjectPairFilter;
        Scoped<JPH::PhysicsSystem>                       mPhysicsSystem;

        std::vector<JPH::BodyID>                        mBodyIds;
        std::vector<JPH::ShapeRefC>                     mBaseShapes;
        std::vector<float>                              mMassOverrides;
        std::unordered_map<std::string, JPH::ShapeRefC> mShapeCache;
        std::deque<uint32_t>                            mFreeSlots;

        bool mBroadPhaseDirty{};

#ifndef NDEBUG
        std::thread::id mOwnerThreadId{std::this_thread::get_id()};
#endif
    };
}
