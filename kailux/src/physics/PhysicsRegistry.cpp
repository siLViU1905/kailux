#include "PhysicsRegistry.h"

#include <glm/gtx/matrix_decompose.hpp>

#include "Log.h"
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <execution>

namespace kailux
{
    PhysicsRegistry::PhysicsRegistry() = default;

    PhysicsRegistry::PhysicsRegistry(PhysicsRegistry &&other) noexcept : mAllocator(std::move(other.mAllocator)),
                                                                         mJobSystem(std::move(other.mJobSystem)),
                                                                         mBroadPhaseLayer(std::move(other.mBroadPhaseLayer)),
                                                                         mObjectVsBroadPhaseLayer(std::move(other.mObjectVsBroadPhaseLayer)),
                                                                         mObjectPairFilter(std::move(other.mObjectPairFilter)),
                                                                         mContactCollector(std::move(other.mContactCollector)),
                                                                         mPhysicsSystem(std::move(other.mPhysicsSystem)),
                                                                         mBodyIds(std::move(other.mBodyIds)),
                                                                         mBaseShapes(std::move(other.mBaseShapes)),
                                                                         mMassOverrides(std::move(other.mMassOverrides)),
                                                                         mShapeCache(std::move(other.mShapeCache)),
                                                                         mFreeSlots(std::move(other.mFreeSlots)),
                                                                         mActiveContacts(std::move(other.mActiveContacts)),
                                                                         mBroadPhaseDirty(other.mBroadPhaseDirty)
#ifndef NDEBUG
                                                                         ,mOwnerThreadId(other.mOwnerThreadId)
#endif
    {
    }

    PhysicsRegistry & PhysicsRegistry::operator=(PhysicsRegistry &&other) noexcept
    {
        if (this != &other)
        {
            mAllocator = std::move(other.mAllocator);
            mJobSystem = std::move(other.mJobSystem);
            mBroadPhaseLayer = std::move(other.mBroadPhaseLayer);
            mObjectVsBroadPhaseLayer = std::move(other.mObjectVsBroadPhaseLayer);
            mObjectPairFilter = std::move(other.mObjectPairFilter);
            mContactCollector = std::move(other.mContactCollector);
            mPhysicsSystem = std::move(other.mPhysicsSystem);
            mBodyIds = std::move(other.mBodyIds);
            mBaseShapes = std::move(other.mBaseShapes);
            mMassOverrides = std::move(other.mMassOverrides);
            mShapeCache = std::move(other.mShapeCache);
            mFreeSlots = std::move(other.mFreeSlots);
            mActiveContacts = std::move(other.mActiveContacts);
            mBroadPhaseDirty = other.mBroadPhaseDirty;
#ifndef NDEBUG
            mOwnerThreadId = other.mOwnerThreadId;
#endif
        }
        return *this;
    }

    PhysicsRegistry PhysicsRegistry::create()
    {
        static bool joltInitialized = false;
        if (!joltInitialized)
        {
            JPH::RegisterDefaultAllocator();
            JPH::Factory::sInstance = new JPH::Factory();
            JPH::RegisterTypes();
            joltInitialized = true;
        }

        log::console.Debug("physics registry: creating");
        PhysicsRegistry registry;
        registry.mAllocator = create_scoped<JPH::TempAllocatorImplWithMallocFallback>(kAllocatorSize);

        auto threads = pick_thread_count(2);
        log::console.Debug("physics registry: created job system with {} threads", threads);
        registry.mJobSystem = create_scoped<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs,
            JPH::cMaxPhysicsBarriers,
            threads
        );

        registry.mBroadPhaseLayer = create_scoped<impl::BroadPhaseLayer>();
        registry.mObjectVsBroadPhaseLayer = create_scoped<impl::ObjectVsBroadPhaseLayerFilter>();
        registry.mObjectPairFilter = create_scoped<impl::ObjectLayerPairFilter>();
        registry.mContactCollector = create_scoped<impl::ContactCollector>();

        registry.mPhysicsSystem = create_scoped<JPH::PhysicsSystem>();
        registry.mPhysicsSystem->Init(
            kMaxBodies,
            kNumBodyMutexes,
            kMaxBodyPairs,
            kMaxContactConstraints,
            *registry.mBroadPhaseLayer,
            *registry.mObjectVsBroadPhaseLayer,
            *registry.mObjectPairFilter
        );
        auto settings{registry.mPhysicsSystem->GetPhysicsSettings()};
        settings.mNumVelocitySteps = 20;
        settings.mNumPositionSteps = 4;
        registry.mPhysicsSystem->SetPhysicsSettings(settings);
        registry.mPhysicsSystem->SetContactListener(registry.mContactCollector.get());

        log::console.Debug("physics registry: physics system initialized");

        registry.AllocResources();

        return registry;
    }

    PhysicsRegistry::BodyResult PhysicsRegistry::CreateBody(const PhysicsBodyInfo &info)
    {
        if (info.meshType == MeshType::Unknown)
            return std::unexpected{"Unknown mesh type"};
        if (!is_valid_pose(info.transform.position, info.transform.rotation))
            return std::unexpected{"Invalid transform (NaN, infinite or zero rotation)"};
        if (!scale_is_usable(info.transform.scale))
            return std::unexpected{"Scale is zero on at least one axis"};

        const auto baseResult{
            (info.meshType != MeshType::Loaded)
                  ? create_builtin_mesh_body(info.meshType)
                  : GetOrCreateLoadedShape(info)
        };
        if (!baseResult)
            return std::unexpected{baseResult.error()};
        const JPH::ShapeRefC base{*baseResult};

        const auto slot{AcquireSlot()};
        if (slot == BodyHandle::kInvalidIndex)
            return std::unexpected{"No more slots"};

        const auto shape{make_scaled_shape(base, info.transform.scale)};

        const auto motionType{static_cast<JPH::EMotionType>(info.options.bodyType)};
        const auto objectLayer{
            (info.options.bodyType == PhysicsBodyType::Static) ? layers::kNonMoving : layers::kMoving
        };

        const auto &transform{info.transform};
        JPH::BodyCreationSettings settings{
            shape,
            {transform.position.x, transform.position.y, transform.position.z},
            {transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w},
            motionType,
            objectLayer
        };

        if (info.options.bodyType != PhysicsBodyType::Static || info.options.canBecomeDynamic)
        {
            settings.mMotionQuality = JPH::EMotionQuality::LinearCast;
            settings.mEnhancedInternalEdgeRemoval = true;
        }

        if (info.options.bodyType == PhysicsBodyType::Static && info.options.canBecomeDynamic)
            settings.mAllowDynamicOrKinematic = true;

        settings.mIsSensor = info.options.isSensor;

        if (info.options.mass > 0.f)
        {
            settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
            settings.mMassPropertiesOverride.mMass = info.options.mass;
        }

        mBodyIds[slot]       = GetBodies().CreateAndAddBody(settings, JPH::EActivation::Activate);
        mBaseShapes[slot]    = base;
        mMassOverrides[slot] = std::max(info.options.mass, 0.f);

        mBroadPhaseDirty = true;

        return BodyHandle{slot};
    }

    void PhysicsRegistry::DestroyBody(BodyHandle handle)
    {
        assert(handle.Valid());
        const auto id{mBodyIds[handle.index]};
        if (id.IsInvalid())
            return;

        auto& bodyInterface = GetBodies();

        if (bodyInterface.IsAdded(id))
            bodyInterface.RemoveBody(id);

        bodyInterface.DestroyBody(id);

        const auto value{id.GetIndexAndSequenceNumber()};
        std::erase_if(mActiveContacts, [value](const auto& entry)
        {
            return entry.first.first == value || entry.first.second == value;
        });

        mBodyIds[handle.index] = JPH::BodyID{};
        mBaseShapes[handle.index] = nullptr;
        mMassOverrides[handle.index] = 0.f;
        mFreeSlots.push_back(handle.index);
    }

    void PhysicsRegistry::SetBodyEnabled(BodyHandle handle, bool enabled)
    {
        assert(handle.Valid());
        auto id = mBodyIds[handle.index];

        auto& bodyInterface = GetBodies();
        bool isAdded = bodyInterface.IsAdded(id);
        if (enabled && !isAdded)
            bodyInterface.AddBody(id, JPH::EActivation::Activate);
        else if (!enabled && isAdded)
            bodyInterface.RemoveBody(id);
    }

    bool PhysicsRegistry::IsBodyEnabled(BodyHandle handle) const
    {
        assert(handle.Valid());
        auto id = mBodyIds[handle.index];
        return GetBodies().IsAdded(id);
    }

    bool PhysicsRegistry::SetBodyType(BodyHandle handle, PhysicsBodyType type)
    {
        assert(handle.Valid());
        const auto id{mBodyIds[handle.index]};

        if (type != PhysicsBodyType::Static)
        {
            const JPH::BodyLockRead lock(GetLocks(), id);
            if (!lock.Succeeded() || !lock.GetBody().CanBeKinematicOrDynamic())
                return false;
        }

        auto& bodyInterface = GetBodies();

        auto motionType = static_cast<JPH::EMotionType>(type);
        auto layer      = (type == PhysicsBodyType::Static) ? layers::kNonMoving : layers::kMoving;

        auto activation = (type == PhysicsBodyType::Dynamic) ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;

        bodyInterface.SetMotionType(id, motionType, activation);

        bodyInterface.SetObjectLayer(id, layer);
        return true;
    }

    void PhysicsRegistry::AddForce(BodyHandle handle, const glm::vec3 &force)
    {
        assert(handle.Valid());
        auto id = mBodyIds[handle.index];
        GetBodies().AddForce(id, {force.x, force.y, force.z});
    }

    void PhysicsRegistry::AddImpulse(BodyHandle handle, const glm::vec3 &impulse)
    {
        assert(handle.Valid());
        auto id = mBodyIds[handle.index];
        GetBodies().AddImpulse(id, {impulse.x, impulse.y, impulse.z});
    }

    void PhysicsRegistry::SetLinearVelocity(BodyHandle handle, const glm::vec3 &velocity)
    {
        assert(handle.Valid());
        auto id = mBodyIds[handle.index];
        GetBodies().SetLinearVelocity(id, {velocity.x, velocity.y, velocity.z});
    }

    glm::vec3 PhysicsRegistry::GetLinearVelocity(BodyHandle handle) const
    {
        assert(handle.Valid());
        auto id = mBodyIds[handle.index];
        auto velocity = GetBodies().GetLinearVelocity(id);
        return {velocity.GetX(), velocity.GetY(), velocity.GetZ()};
    }

    void PhysicsRegistry::SetAngularVelocity(BodyHandle handle, const glm::vec3 &velocity)
    {
        assert(handle.Valid());
        const auto id{mBodyIds[handle.index]};
        GetBodies().SetAngularVelocity(id, {velocity.x, velocity.y, velocity.z});
    }

    glm::vec3 PhysicsRegistry::GetAngularVelocity(BodyHandle handle) const
    {
        assert(handle.Valid());
        const auto id{mBodyIds[handle.index]};
        const auto velocity{GetBodies().GetAngularVelocity(id)};
        return {velocity.GetX(), velocity.GetY(), velocity.GetZ()};
    }

    void PhysicsRegistry::ResetVelocities(BodyHandle handle)
    {
        assert(handle.Valid());
        GetBodies().SetLinearAndAngularVelocity(
            mBodyIds[handle.index],
            JPH::Vec3::sZero(),
            JPH::Vec3::sZero()
        );
    }

    void PhysicsRegistry::SetBodyUserData(BodyHandle handle, uint64_t userData)
    {
        assert(handle.Valid());
        GetBodies().SetUserData(mBodyIds[handle.index], userData);
    }

    uint64_t PhysicsRegistry::GetBodyUserData(BodyHandle handle) const
    {
        assert(handle.Valid());
        return GetBodies().GetUserData(mBodyIds[handle.index]);
    }

    float PhysicsRegistry::GetBodyMass(BodyHandle handle) const
    {
        assert(handle.Valid());
        const JPH::BodyLockRead lock{GetLocks(), mBodyIds[handle.index]};
        if (!lock.Succeeded())
            return 0.f;

        const auto* motion{lock.GetBody().GetMotionPropertiesUnchecked()};
        if (motion == nullptr || motion->GetInverseMassUnchecked() <= 0.f)
            return 0.f;

        return 1.f / motion->GetInverseMassUnchecked();
    }

    namespace details
    {
        class RaycastBodyFilter final : public JPH::BodyFilter
        {
        public:
            explicit RaycastBodyFilter(JPH::BodyID ignored) : mIgnored(ignored)
            {
            }

            bool ShouldCollide(const JPH::BodyID &inBodyID) const override
            {
                return inBodyID != mIgnored;
            }
            bool ShouldCollideLocked(const JPH::Body &inBody) const override
            {
                return !inBody.IsSensor();
            }

        private:
            JPH::BodyID mIgnored;
        };
    }

    std::optional<RaycastResult> PhysicsRegistry::CastRay(const glm::vec3 &origin, const glm::vec3 &direction,
        float maxDistance, BodyHandle ignore) const
    {
        const auto length{glm::length(direction)};
        if (length < 1e-6f || maxDistance <= 0.f)
            return std::nullopt;

        const auto step{direction / length * maxDistance};
        const JPH::RRayCast ray{{origin.x, origin.y, origin.z}, {step.x, step.y, step.z}};

        const details::RaycastBodyFilter filter{ignore.Valid() ? mBodyIds[ignore.index] : JPH::BodyID{}};

        JPH::RayCastResult hit;
        if (!mPhysicsSystem->GetNarrowPhaseQueryNoLock().CastRay(ray, hit, {}, {}, filter))
            return std::nullopt;

        const auto point{ray.GetPointOnRay(hit.mFraction)};

        RaycastResult result{
            0,
            {point.GetX(), point.GetY(), point.GetZ()},
            {0.f, 0.f, 0.f},
            hit.mFraction * maxDistance
        };

        const JPH::BodyLockRead lock{GetLocks(), hit.mBodyID};
        if (lock.Succeeded())
        {
            const auto& body{lock.GetBody()};
            const auto normal{body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, point)};
            result.normal   = {normal.GetX(), normal.GetY(), normal.GetZ()};
            result.userData = body.GetUserData();
        }
        return result;
    }

    std::vector<ContactEvent> PhysicsRegistry::DrainContactEvents()
    {
        std::vector<ContactEvent> events;
        const auto& bodies{GetBodies()};

        for (const auto& record : mContactCollector->Drain())
        {
            auto first{record.first};
            auto second{record.second};
            if (second.GetIndexAndSequenceNumber() < first.GetIndexAndSequenceNumber())
                std::swap(first, second);

            const std::pair key{first.GetIndexAndSequenceNumber(), second.GetIndexAndSequenceNumber()};

            if (record.began)
            {
                if (!bodies.IsAdded(first) || !bodies.IsAdded(second))
                    continue;

                auto& contact{mActiveContacts[key]};
                contact.sensor = contact.sensor || record.sensor;
                if (++contact.count == 1)
                    events.emplace_back(bodies.GetUserData(first), bodies.GetUserData(second), contact.sensor, true);
                continue;
            }
            const auto it{mActiveContacts.find(key)};
            if (it == mActiveContacts.end())
                continue;

            if (--it->second.count == 0)
            {
                events.emplace_back(bodies.GetUserData(first), bodies.GetUserData(second), it->second.sensor, false);
                mActiveContacts.erase(it);
            }
        }
        return events;
    }

    void PhysicsRegistry::SetBodyTransform(BodyHandle handle, const glm::vec3 &position, const glm::quat &rotation)
    {
        assert(handle.Valid());
        if (!is_valid_pose(position, rotation))
            return;

        auto id = mBodyIds[handle.index];

        auto& bodyInterface = GetBodies();

        auto activation = (bodyInterface.GetMotionType(id) == JPH::EMotionType::Dynamic)
                                    ? JPH::EActivation::Activate
                                    : JPH::EActivation::DontActivate;

        const auto unit{glm::normalize(rotation)};

        bodyInterface.SetPositionAndRotation(
            id,
            {position.x, position.y, position.z},
            {unit.x, unit.y, unit.z, unit.w},
            activation
        );
    }

    void PhysicsRegistry::GetBodyTransform(BodyHandle handle, glm::vec3 &outPosition, glm::quat &outRotation) const
    {
        assert(handle.Valid());
        const auto id{mBodyIds[handle.index]};

        JPH::RVec3 pos;
        JPH::Quat rot;
        GetBodies().GetPositionAndRotation(id, pos, rot);

        outPosition = {pos.GetX(), pos.GetY(), pos.GetZ()};
        outRotation = {rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ()};
    }

    void PhysicsRegistry::UpdateBodyScale(BodyHandle handle, const glm::vec3 &scale)
    {
        assert(handle.Valid());
        const auto id{mBodyIds[handle.index]};
        const auto& base{mBaseShapes[handle.index]};
        if (id.IsInvalid() || base == nullptr || !scale_is_usable(scale))
            return;

        const auto& bodyInterface{GetBodies()};
        const auto activation{
            bodyInterface.GetMotionType(id) == JPH::EMotionType::Dynamic
            ? JPH::EActivation::Activate
            : JPH::EActivation::DontActivate
        };

        bodyInterface.SetShape(id, make_scaled_shape(base, scale), true, activation);

        if (const auto mass{mMassOverrides[handle.index]}; mass > 0.f)
            ApplyMassOverride(id, mass);
    }

    void PhysicsRegistry::SetBodyMass(BodyHandle handle, float mass)
    {
        assert(handle.Valid());
        const auto id{mBodyIds[handle.index]};
        if (id.IsInvalid())
            return;

        mMassOverrides[handle.index] = std::max(mass, 0.f);
        ApplyMassOverride(id, mMassOverrides[handle.index]);
    }

    JPH::EPhysicsUpdateError PhysicsRegistry::Update(float deltaTime, int collisionSteps)
    {
        return mPhysicsSystem->Update(
            deltaTime,
            collisionSteps,
            mAllocator.get(),
            mJobSystem.get()
            );
    }

    void PhysicsRegistry::MoveKinematic(
        BodyHandle handle,
        const glm::vec3 &position,
        const glm::quat &rotation,
        float deltaTime
    )
    {
        assert(handle.Valid());
        if (!is_valid_pose(position, rotation))
            return;

        const auto unit{glm::normalize(rotation)};

        GetBodies().MoveKinematic(
            mBodyIds[handle.index],
            {position.x, position.y, position.z},
            {unit.x, unit.y, unit.z, unit.w},
            deltaTime
        );
    }

    void PhysicsRegistry::PruneShapeCache()
    {
        std::erase_if(mShapeCache, [](const auto& entry)
        {
            return entry.second->GetRefCount() == 1;
        });
    }

    void PhysicsRegistry::OptimizeBroadPhaseIfNeeded()
    {
        if (!mBroadPhaseDirty)
            return;
        mPhysicsSystem->OptimizeBroadPhase();
        mBroadPhaseDirty = false;
    }

    uint32_t PhysicsRegistry::pick_thread_count(uint32_t freeThreads)
    {
        uint32_t threads = 1;
        uint32_t maxThreads = std::thread::hardware_concurrency();
        if (maxThreads > freeThreads)
            threads = maxThreads - freeThreads;
        return threads;
    }

    bool PhysicsRegistry::is_valid_pose(const glm::vec3 &position, const glm::quat &rotation)
    {
        const glm::vec4 rot{rotation.x, rotation.y, rotation.z, rotation.w};

        const bool finite{
            !glm::any(glm::isnan(position)) && !glm::any(glm::isinf(position)) &&
            !glm::any(glm::isnan(rot)) && !glm::any(glm::isinf(rot))
        };
        return finite && glm::length(rot) > 1e-6f;
    }

    bool PhysicsRegistry::is_degenerate_triangle(const JPH::Float3 &a, const JPH::Float3 &b, const JPH::Float3 &c)
    {
        const JPH::Vec3 v0{a};
        const JPH::Vec3 v1{b};
        const JPH::Vec3 v2{c};

        return (v1 - v0).Cross(v2 - v0).IsNearZero();
    }

    bool PhysicsRegistry::scale_is_usable(const glm::vec3 &scale)
    {
        constexpr auto kEps{1e-6f};

        return std::abs(scale.x) > kEps
               && std::abs(scale.y) > kEps
               && std::abs(scale.z) > kEps;
    }

    JPH::ShapeRefC PhysicsRegistry::make_scaled_shape(const JPH::ShapeRefC &base, const glm::vec3 &scale)
    {
        const auto absScale{glm::abs(scale)};

        switch (base->GetSubType())
        {
            case JPH::EShapeSubType::Sphere:
            {
                const auto radius{static_cast<const JPH::SphereShape*>(base.GetPtr())->GetRadius()};
                return new JPH::SphereShape{
                    radius * std::max({absScale.x, absScale.y, absScale.z})
                };
            }
            case JPH::EShapeSubType::Box:
            {
                const auto* box{static_cast<const JPH::BoxShape*>(base.GetPtr())};
                const auto half{box->GetHalfExtent() * JPH::Vec3{absScale.x, absScale.y, absScale.z}};
                return new JPH::BoxShape{
                    half,
                    std::min(JPH::cDefaultConvexRadius, half.ReduceMin())
                };
            }
            default:
            {
                JPH::Vec3 joltScale{scale.x, scale.y, scale.z};
                if (joltScale.IsClose(JPH::Vec3::sReplicate(1.f)))
                    return base;
                if (!base->IsValidScale(joltScale))
                    joltScale = base->MakeScaleValid(joltScale);
                return new JPH::ScaledShape{base, joltScale};
            }
        }
    }

    void PhysicsRegistry::ApplyMassOverride(JPH::BodyID id, float mass)
    {
        const JPH::BodyLockWrite lock{GetLocks(), id};
        if (!lock.Succeeded())
            return;

        auto& body{lock.GetBody()};
        auto* motion{body.GetMotionPropertiesUnchecked()};
        if (motion == nullptr)
            return;

        auto props{body.GetShape()->GetMassProperties()};
        if (mass > 0.f)
            props.ScaleToMass(mass);
        motion->SetMassProperties(motion->GetAllowedDOFs(), props);
    }

    void PhysicsRegistry::AllocResources()
    {
        mBodyIds.resize(kMaxBodies, JPH::BodyID(JPH::BodyID::cInvalidBodyID));
        mBaseShapes.resize(kMaxBodies);
        mMassOverrides.resize(kMaxBodies, 0.f);

        for (uint32_t i = 0; i < kMaxBodies; ++i)
            mFreeSlots.push_back(i);
    }

    uint32_t PhysicsRegistry::AcquireSlot()
    {
        if (mFreeSlots.empty())
            return BodyHandle::kInvalidIndex;
        auto slot = mFreeSlots.front();
        mFreeSlots.pop_front();
        return slot;
    }

    PhysicsRegistry::CreateResult PhysicsRegistry::GetOrCreateLoadedShape(const PhysicsBodyInfo &info)
    {
        if (!info.mesh)
            return std::unexpected{"Loaded mesh has no physics data"};

        const bool triangleMesh{
            info.options.bodyType == PhysicsBodyType::Static && !info.options.canBecomeDynamic
        };
        auto key{std::format("{}#{}", info.mesh->key, triangleMesh ? "mesh" : "hull")};

        if (const auto it{mShapeCache.find(key)}; it != mShapeCache.end())
            return it->second;

        auto result{
            triangleMesh
                ? create_loaded_mesh_body(info)
                : create_single_hull(*info.mesh)
        };
        if (result)
            mShapeCache.emplace(std::move(key), *result);
        return result;
    }

    PhysicsRegistry::CreateResult PhysicsRegistry::create_builtin_mesh_body(MeshType type)
    {
        JPH::ShapeRefC shape;
        switch (type)
        {
            case MeshType::Cube:
            {
                shape = new JPH::BoxShape(JPH::Vec3::sReplicate(0.5f));
                break;
            }
            case MeshType::Sphere:
            {
                shape = new JPH::SphereShape(0.5f);
                break;
            }
            case MeshType::Loaded:
                break;
            case MeshType::Unknown:
                break;
        }
        return shape;
    }

    PhysicsRegistry::CreateResult PhysicsRegistry::create_loaded_mesh_body(const PhysicsBodyInfo &info)
    {
        if (!info.mesh || info.mesh->submeshes.empty())
            return std::unexpected{"physics registry: loaded mesh has no physics data"};

        const auto submeshesCount{info.mesh->submeshes.size()};

        std::vector<size_t> indices(submeshesCount);
        std::iota(indices.begin(), indices.end(), size_t{});

        std::vector<ChildShapeResult> results(submeshesCount);
        std::vector<BuildResult> buildResults(submeshesCount);

        std::for_each(std::execution::par, indices.begin(), indices.end(),
            [&info, &results, &buildResults](auto idx)
            {
                buildResults[idx] = build_submesh_shape(info, idx, results[idx]);
            });

        uint32_t failed{};
        for (const auto &result : buildResults)
            if (!result)
            {
                ++failed;
                log::console.Warning("physics registry: {}", result.error());
            }

        std::vector<const ChildShapeResult*> valid;
        valid.reserve(results.size());
        for (auto &result: results)
            if (result.shape != nullptr)
                valid.emplace_back(&result);

        if (failed > 0)
            log::console.Warning("physics registry: {}/{} submeshes skipped",failed, submeshesCount);

        if (valid.empty())
        {
            constexpr std::string_view kErrorMsg{"physics registry: no valid submesh shape, body not created"};
            log::console.Error(kErrorMsg);
            return std::unexpected{kErrorMsg.data()};
        }

        if (valid.size() == 1uz)
        {
            const auto &shape{*valid.front()};

            if (shape.trans.IsNearZero() && shape.rot.IsClose(JPH::Quat::sIdentity()))
                return shape.shape;

            const JPH::RotatedTranslatedShapeSettings settings{shape.trans, shape.rot, shape.shape};
            settings.SetEmbedded();

            const auto result{settings.Create()};
            if (result.HasError())
            {
                const auto errorMsg{std::format("physics registry: RotatedTranslatedShape Jolt error: {}", result.GetError().c_str())};
                log::console.Error("{}", errorMsg);
                return std::unexpected{errorMsg};
            }
            return result.Get();
        }

        JPH::StaticCompoundShapeSettings compoundSettings;
        compoundSettings.SetEmbedded();
        for (const auto *result: valid)
            compoundSettings.AddShape(result->trans, result->rot, result->shape);

        const auto result{compoundSettings.Create()};
        if (result.HasError())
        {
            const auto errorMsg{std::format("physics registry: StaticCompoundShape Jolt error: {}", result.GetError().c_str())};
            log::console.Error("{}", errorMsg);
            return std::unexpected{errorMsg};
        }
        return result.Get();
    }

    PhysicsRegistry::CreateResult PhysicsRegistry::create_single_hull(const PhysicsMeshData &mesh)
    {
        size_t total{};
        for (const auto& submesh : mesh.submeshes)
            total += submesh.positions.size();

        JPH::Array<JPH::Vec3> points;
        points.reserve(total);

        glm::vec3 low{std::numeric_limits<float>::max()};
        glm::vec3 high{std::numeric_limits<float>::lowest()};
        for (const auto& submesh : mesh.submeshes)
            for (const auto& position : submesh.positions)
            {
                const glm::vec3 point{submesh.localTransform * glm::vec4{position, 1.f}};
                points.emplace_back(point.x, point.y, point.z);
                low = glm::min(low, point);
                high = glm::max(high, point);
            }

        if (points.size() < 4)
            return std::unexpected{"physics registry: not enough points for a convex hull"};

        JPH::ConvexHullShapeSettings settings{points};
        settings.mHullTolerance = std::max(1e-3f, 0.005f * glm::length(high - low));
        settings.SetEmbedded();

        const auto result{settings.Create()};
        if (result.HasError())
            return std::unexpected{std::format("physics registry: ConvexHull Jolt error: {}",
                                          result.GetError().c_str())};
        return result.Get();
    }

    const JPH::BodyLockInterface & PhysicsRegistry::GetLocks() const
    {
        assert(std::this_thread::get_id() == mOwnerThreadId && "PhysicsRegistry used off the main thread");
        return mPhysicsSystem->GetBodyLockInterfaceNoLock();
    }

    PhysicsRegistry::BuildResult PhysicsRegistry::build_submesh_shape(const PhysicsBodyInfo &info, size_t idx, ChildShapeResult &out)
    {
        out = {};

        const auto& submesh{info.mesh->submeshes[idx]};

        const auto submeshFormatString{std::format("submesh {}: ", idx)};

        if (submesh.positions.empty())
            return std::unexpected{submeshFormatString + "Shape has no vertices"};
        if (submesh.indices.size() < 3)
            return std::unexpected{submeshFormatString + "Shape has less than 3 indices"};


        glm::vec3 lScale, lTrans, lSkew; glm::quat lRot; glm::vec4 lPersp;
        if (!glm::decompose(submesh.localTransform, lScale, lRot, lTrans, lSkew, lPersp))
            return std::unexpected{submeshFormatString + "Transform could not be decomposed"};

        if (!scale_is_usable(lScale))
            return std::unexpected{
                submeshFormatString + std::format("Scale (x: {:.3f}, y:{:.3f}, z: {:.3f}) is not usable",
                                                  lScale.x,
                                                  lScale.y,
                                                  lScale.z)
            };

        const bool useTriangleMesh{
            info.options.bodyType == PhysicsBodyType::Static
            && !info.options.canBecomeDynamic
        };

        JPH::ShapeRefC childShape;
        if (useTriangleMesh)
        {
            JPH::VertexList joltVertices;
            joltVertices.reserve(submesh.positions.size());
            for (const auto& position : submesh.positions)
                joltVertices.emplace_back(
                    position.x,
                    position.y,
                    position.z
                );

            const auto vertexCount{static_cast<uint32_t>(joltVertices.size())};
            const auto triangleCount{submesh.indices.size() / 3uz};

            JPH::IndexedTriangleList joltTriangles;
            joltTriangles.reserve(triangleCount);

            uint32_t skippedDegenerate{};
            uint32_t skippedOutOfRange{};

            for (size_t t{}; t < triangleCount; ++t)
            {
                const auto i0{submesh.indices[t * 3 + 0]};
                const auto i1{submesh.indices[t * 3 + 1]};
                const auto i2{submesh.indices[t * 3 + 2]};

                if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount)
                {
                    ++skippedOutOfRange;
                    continue;
                }

                if (i0 == i1 || i1 == i2 || i0 == i2)
                {
                    ++skippedDegenerate;
                    continue;
                }

                if (is_degenerate_triangle(joltVertices[i0], joltVertices[i1], joltVertices[i2]))
                {
                    ++skippedDegenerate;
                    continue;
                }

                joltTriangles.emplace_back(i0, i1, i2);
            }

            if (joltTriangles.empty())
                return std::unexpected{
                    submeshFormatString + std::format("submesh {}: 0 valid triangles ({} degenerate, {} out of range)",
                                                      idx, skippedDegenerate, skippedOutOfRange)
                };

            const JPH::MeshShapeSettings settings{std::move(joltVertices), std::move(joltTriangles)};
            settings.SetEmbedded();

            const auto result{settings.Create()};
            if (result.HasError())
                return std::unexpected{submeshFormatString + std::format("MeshShape Jolt error: {}", result.GetError().c_str())};
            childShape = result.Get();
        }
        else
        {
            JPH::Array<JPH::Vec3> hullVertices;
            hullVertices.reserve(submesh.positions.size());
            for (const auto& position : submesh.positions)
                hullVertices.emplace_back(
                    position.x,
                    position.y,
                    position.z
                );

            if (hullVertices.size() < 3)
                return std::unexpected{submeshFormatString + "hull has 0 vertices"};

            const JPH::ConvexHullShapeSettings settings{hullVertices};
            settings.SetEmbedded();

            const auto result{settings.Create()};
            if (result.HasError())
                return std::unexpected{submeshFormatString + std::format("ConvexHull Jolt error: {}", result.GetError().c_str())};
            childShape = result.Get();
        }

        if (lScale != glm::vec3{1.f})
        {
            const JPH::ScaledShapeSettings settings{childShape, {lScale.x, lScale.y, lScale.z}};
            settings.SetEmbedded();

            const auto result{settings.Create()};
            if (result.HasError())
                return std::unexpected{submeshFormatString + std::format("ScaledShape Jolt error: {}", result.GetError().c_str())};
            childShape = result.Get();
        }

        out.shape = childShape;
        out.trans = {lTrans.x, lTrans.y, lTrans.z};
        out.rot   = {lRot.x, lRot.y, lRot.z, lRot.w};

        return {};
    }
}
