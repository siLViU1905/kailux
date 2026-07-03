#include "PhysicsRegistry.h"

#include <glm/gtx/matrix_decompose.hpp>

#include "core/Log.h"
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>

namespace kailux
{
    PhysicsRegistry::PhysicsRegistry() = default;

    PhysicsRegistry::PhysicsRegistry(PhysicsRegistry &&other) noexcept : mAllocator(std::move(other.mAllocator)),
                                                                         mJobSystem(std::move(other.mJobSystem)),
                                                                         mBroadPhaseLayer(std::move(other.mBroadPhaseLayer)),
                                                                         mObjectVsBroadPhaseLayer(std::move(other.mObjectVsBroadPhaseLayer)),
                                                                         mObjectPairFilter(std::move(other.mObjectPairFilter)),
                                                                         mPhysicsSystem(std::move(other.mPhysicsSystem)),
                                                                         mBodyIds(std::move(other.mBodyIds)),
                                                                         mFreeSlots(std::move(other.mFreeSlots))
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
            mPhysicsSystem = std::move(other.mPhysicsSystem);
            mBodyIds = std::move(other.mBodyIds);
            mFreeSlots = std::move(other.mFreeSlots);
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

        KAILUX_LOG_PARENT_CLR_MAGENTA("[PhysicsRegistry]")
        PhysicsRegistry registry;
        registry.mAllocator = create_scoped<JPH::TempAllocatorImpl>(kAllocatorSize);

        auto threads = pick_thread_count(2);
        KAILUX_LOG_CHILD_CLR_MAGENTA(std::format("Created job system with {} threads", threads))
        registry.mJobSystem = create_scoped<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs,
            JPH::cMaxPhysicsBarriers,
            threads
        );

        registry.mBroadPhaseLayer = create_scoped<impl::BroadPhaseLayer>();
        registry.mObjectVsBroadPhaseLayer = create_scoped<impl::ObjectVsBroadPhaseLayerFilter>();
        registry.mObjectPairFilter = create_scoped<impl::ObjectLayerPairFilter>();
        registry.mPhysicsSystem = create_scoped<JPH::PhysicsSystem>();

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
        KAILUX_LOG_CHILD_CLR_MAGENTA("Physics system initialized")

        registry.allocResources();

        return registry;
    }

    BodyHandle PhysicsRegistry::createBody(const PhysicsBodyInfo &info)
    {
        auto slot = acquireSlot();
        if (slot == BodyHandle::kInvalidIndex)
            return {};

        JPH::ShapeRefC shape;
        if (info.meshType == MeshType::Unknown)
            return {};
        if (info.meshType != MeshType::Loaded)
            shape = create_builtin_mesh_body(info.meshType, info.transform);
        else
            shape = create_loaded_mesh_body(info);

        auto motionType = static_cast<JPH::EMotionType>(info.options.bodyType);
        auto objectLayer = (info.options.bodyType == PhysicsBodyType::Static) ? layers::kNonMoving : layers::kMoving;

        const auto& transform = info.transform;
        JPH::BodyCreationSettings settings(
            shape,
            {transform.position.x, transform.position.y, transform.position.z},
            {transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w},
            motionType,
            objectLayer
            );
        if (info.options.bodyType == PhysicsBodyType::Static && info.options.canBecomeDynamic)
            settings.mAllowDynamicOrKinematic = true;

        mBodyIds[slot] = mPhysicsSystem->GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::Activate);
        return {slot};
    }

    void PhysicsRegistry::destroyBody(BodyHandle handle)
    {
        assert(handle.valid());
        auto id = mBodyIds[handle.index];

        auto& bodyInterface = mPhysicsSystem->GetBodyInterface();

        if (bodyInterface.IsAdded(id))
            bodyInterface.RemoveBody(id);

        bodyInterface.DestroyBody(id);

        mBodyIds[handle.index] = JPH::BodyID(JPH::BodyID::cInvalidBodyID);
        mFreeSlots.push_back(handle.index);
    }

    void PhysicsRegistry::setBodyEnabled(BodyHandle handle, bool enabled)
    {
        assert(handle.valid());
        auto id = mBodyIds[handle.index];

        auto& bodyInterface = mPhysicsSystem->GetBodyInterface();
        bool isAdded = bodyInterface.IsAdded(id);
        if (enabled && !isAdded)
            bodyInterface.AddBody(id, JPH::EActivation::Activate);
        else if (!enabled && isAdded)
            bodyInterface.RemoveBody(id);
    }

    bool PhysicsRegistry::isBodyEnabled(BodyHandle handle) const
    {
        assert(handle.valid());
        auto id = mBodyIds[handle.index];
        return mPhysicsSystem->GetBodyInterface().IsAdded(id);
    }

    void PhysicsRegistry::setBodyType(BodyHandle handle, PhysicsBodyType type)
    {
        assert(handle.valid());
        auto id = mBodyIds[handle.index];
        auto& bodyInterface = mPhysicsSystem->GetBodyInterface();

        auto motionType = static_cast<JPH::EMotionType>(type);
        auto layer      = (type == PhysicsBodyType::Static) ? layers::kNonMoving : layers::kMoving;

        auto activation = (type == PhysicsBodyType::Dynamic) ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;

        bodyInterface.SetMotionType(id, motionType, activation);

        bodyInterface.SetObjectLayer(id, layer);
    }

    void PhysicsRegistry::addForce(BodyHandle handle, const glm::vec3 &force)
    {
        assert(handle.valid());
        auto id = mBodyIds[handle.index];
        mPhysicsSystem->GetBodyInterface().AddForce(id, {force.x, force.y, force.z});
    }

    void PhysicsRegistry::addImpulse(BodyHandle handle, const glm::vec3 &impulse)
    {
        assert(handle.valid());
        auto id = mBodyIds[handle.index];
        mPhysicsSystem->GetBodyInterface().AddImpulse(id, {impulse.x, impulse.y, impulse.z});
    }

    void PhysicsRegistry::setLinearVelocity(BodyHandle handle, const glm::vec3 &velocity)
    {
        assert(handle.valid());
        auto id = mBodyIds[handle.index];
        mPhysicsSystem->GetBodyInterface().AddImpulse(id, {velocity.x, velocity.y, velocity.z});
    }

    glm::vec3 PhysicsRegistry::getLinearVelocity(BodyHandle handle) const
    {
        assert(handle.valid());
        auto id = mBodyIds[handle.index];
        auto velocity = mPhysicsSystem->GetBodyInterface().GetLinearVelocity(id);
        return {velocity.GetX(), velocity.GetY(), velocity.GetZ()};
    }

    void PhysicsRegistry::setBodyTransform(BodyHandle handle, const glm::vec3 &position, const glm::quat &rotation)
    {
        assert(handle.valid());
        auto id = mBodyIds[handle.index];

        auto& bodyInterface = mPhysicsSystem->GetBodyInterface();

        JPH::Vec3 joltPos(position.x, position.y, position.z);

        JPH::Quat joltRot(rotation.x, rotation.y, rotation.z, rotation.w);

        auto activation = (bodyInterface.GetMotionType(id) == JPH::EMotionType::Dynamic)
                                    ? JPH::EActivation::Activate
                                    : JPH::EActivation::DontActivate;

        bodyInterface.SetPositionAndRotation(id, joltPos, joltRot, activation);
    }

    void PhysicsRegistry::getBodyTransform(BodyHandle handle, glm::vec3 &outPosition, glm::quat &outRotation) const
    {
        assert(handle.valid());
        auto id = mBodyIds[handle.index];

        JPH::BodyLockRead lock(mPhysicsSystem->GetBodyLockInterface(), id);
        if (lock.Succeeded())
        {
            const auto& body = lock.GetBody();

            auto pos = body.GetPosition();
            outPosition = {pos.GetX(), pos.GetY(), pos.GetZ()};

            auto rot = body.GetRotation();
            outRotation = {rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ()};
        }
    }

    void PhysicsRegistry::updateBodyScale(BodyHandle handle, const glm::vec3 &scale)
    {
        assert(handle.valid());
        auto id = mBodyIds[handle.index];

        auto& bodyInterface = mPhysicsSystem->GetBodyInterface();
        JPH::ShapeRefC currentShape = bodyInterface.GetShape(id);
        auto baseShape = currentShape;

        if (currentShape->GetSubType() == JPH::EShapeSubType::Scaled)
        {
            const auto* scaledShape = static_cast<const JPH::ScaledShape*>(currentShape.GetPtr());
            baseShape = scaledShape->GetInnerShape();
        }

        JPH::ShapeRefC newShape;
        if (baseShape->GetSubType() == JPH::EShapeSubType::Sphere)
        {
            const auto* sphereShape = static_cast<const JPH::SphereShape*>(baseShape.GetPtr());

            float maxScale = std::max({scale.x, scale.y, scale.z});
            newShape = new JPH::SphereShape(sphereShape->GetRadius() * maxScale);
        }
        else if (baseShape->GetSubType() == JPH::EShapeSubType::Box)
        {
            const auto* boxShape = static_cast<const JPH::BoxShape*>(baseShape.GetPtr());
            JPH::Vec3 originalHalfExtents = boxShape->GetHalfExtent();

            newShape = new JPH::BoxShape(JPH::Vec3(
                originalHalfExtents.GetX() * scale.x,
                originalHalfExtents.GetY() * scale.y,
                originalHalfExtents.GetZ() * scale.z
            ));
        }
        else
            newShape = new JPH::ScaledShape(baseShape, {scale.x, scale.y, scale.z});

        bodyInterface.SetShape(
            id,
            newShape,
            true,
            JPH::EActivation::Activate
            );
    }

    void PhysicsRegistry::update(float deltaTime)
    {
        mPhysicsSystem->Update(
            deltaTime,
            kCollisionSteps,
            mAllocator.get(),
            mJobSystem.get()
            );
    }

    uint32_t PhysicsRegistry::pick_thread_count(uint32_t freeThreads)
    {
        uint32_t threads = 1;
        uint32_t maxThreads = std::thread::hardware_concurrency();
        if (maxThreads > freeThreads)
            threads = maxThreads - freeThreads;
        return threads;
    }

    void PhysicsRegistry::allocResources()
    {
        mBodyIds.resize(kMaxBodies, JPH::BodyID(JPH::BodyID::cInvalidBodyID));

        for (uint32_t i = 0; i < kMaxBodies; ++i)
            mFreeSlots.push_back(i);
    }

    uint32_t PhysicsRegistry::acquireSlot()
    {
        if (mFreeSlots.empty())
            return BodyHandle::kInvalidIndex;
        auto slot = mFreeSlots.front();
        mFreeSlots.pop_front();
        return slot;
    }

    JPH::ShapeRefC PhysicsRegistry::create_builtin_mesh_body(MeshType type, const MeshTransformData &transform)
    {
        JPH::ShapeRefC shape;
        switch (type)
        {
            case MeshType::Cube:
            {
                JPH::Vec3 halfExtents(transform.scale.x * 0.5f, transform.scale.y * 0.5f, transform.scale.z * 0.5f);
                shape = new JPH::BoxShape(halfExtents);
                break;
            }
            case MeshType::Sphere:
            {
                float radius = transform.scale.x * 0.5f;
                shape = new JPH::SphereShape(radius);
                break;
            }
            case MeshType::Loaded:
                break;
            case MeshType::Unknown:
                break;
        }
        return shape;
    }

    JPH::ShapeRefC PhysicsRegistry::create_loaded_mesh_body(const PhysicsBodyInfo &info)
    {
        JPH::StaticCompoundShapeSettings compoundSettings;
        for (const auto& submesh: info.submeshes)
        {
            glm::vec3 lScale, lTrans, lSkew; glm::quat lRot; glm::vec4 lPersp;
            glm::decompose(submesh.localTransform, lScale, lRot, lTrans, lSkew, lPersp);

            JPH::VertexList joltVertices;
            joltVertices.reserve(submesh.vertices.size());
            for (const auto& vertex:submesh.vertices)
                joltVertices.emplace_back(
                    vertex.position.x * lScale.x,
                    vertex.position.y * lScale.y,
                    vertex.position.z * lScale.z
                    );

            JPH::ShapeRefC childShape;
            if (info.options.bodyType == PhysicsBodyType::Static && !info.options.canBecomeDynamic)
            {
                JPH::IndexedTriangleList joltTriangles;
                joltTriangles.reserve(submesh.indices.size() / 3);
                for (size_t i = 0;i<submesh.indices.size();i+=3)
                    joltTriangles.emplace_back(
                        submesh.indices[i],
                        submesh.indices[i + 1],
                        submesh.indices[i + 2]
                        );
                childShape = JPH::MeshShapeSettings(joltVertices, joltTriangles).Create().Get();
            }
            else
            {
                JPH::Array<JPH::Vec3> hullVertices;
                hullVertices.reserve(submesh.vertices.size());
                for (const auto& vertex : submesh.vertices)
                    hullVertices.emplace_back(
                        vertex.position.x * lScale.x,
                        vertex.position.y * lScale.y,
                        vertex.position.z * lScale.z
                    );
                childShape = JPH::ConvexHullShapeSettings(hullVertices).Create().Get();
            }

            if (childShape)
                compoundSettings.AddShape(
                    {lTrans.x, lTrans.y, lTrans.z},
                    {lRot.x, lRot.y, lRot.z, lRot.w},
                    childShape
                    );
        }
        return compoundSettings.Create().Get();
    }
}
