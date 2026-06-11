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

    PhysicsRegistry::PhysicsRegistry(PhysicsRegistry &&other) noexcept : m_Allocator(std::move(other.m_Allocator)),
                                                                         m_JobSystem(std::move(other.m_JobSystem)),
                                                                         m_BroadPhaseLayer(std::move(other.m_BroadPhaseLayer)),
                                                                         m_ObjectVsBroadPhaseLayer(std::move(other.m_ObjectVsBroadPhaseLayer)),
                                                                         m_ObjectPairFilter(std::move(other.m_ObjectPairFilter)),
                                                                         m_PhysicsSystem(std::move(other.m_PhysicsSystem)),
                                                                         m_BodyIds(std::move(other.m_BodyIds)),
                                                                         m_FreeSlots(std::move(other.m_FreeSlots))
    {
    }

    PhysicsRegistry & PhysicsRegistry::operator=(PhysicsRegistry &&other) noexcept
    {
        if (this != &other)
        {
            m_Allocator = std::move(other.m_Allocator);
            m_JobSystem = std::move(other.m_JobSystem);
            m_BroadPhaseLayer = std::move(other.m_BroadPhaseLayer);
            m_ObjectVsBroadPhaseLayer = std::move(other.m_ObjectVsBroadPhaseLayer);
            m_ObjectPairFilter = std::move(other.m_ObjectPairFilter);
            m_PhysicsSystem = std::move(other.m_PhysicsSystem);
            m_BodyIds = std::move(other.m_BodyIds);
            m_FreeSlots = std::move(other.m_FreeSlots);
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
        registry.m_Allocator = create_scoped<JPH::TempAllocatorImpl>(s_AllocatorSize);

        auto threads = pick_thread_count(2);
        KAILUX_LOG_CHILD_CLR_MAGENTA(std::format("Created job system with {} threads", threads))
        registry.m_JobSystem = create_scoped<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs,
            JPH::cMaxPhysicsBarriers,
            threads
        );

        registry.m_BroadPhaseLayer = create_scoped<impl::BroadPhaseLayer>();
        registry.m_ObjectVsBroadPhaseLayer = create_scoped<impl::ObjectVsBroadPhaseLayerFilter>();
        registry.m_ObjectPairFilter = create_scoped<impl::ObjectLayerPairFilter>();
        registry.m_PhysicsSystem = create_scoped<JPH::PhysicsSystem>();

        registry.m_PhysicsSystem = create_scoped<JPH::PhysicsSystem>();
        registry.m_PhysicsSystem->Init(
            s_MaxBodies,
            s_NumBodyMutexes,
            s_MaxBodyPairs,
            s_MaxContactConstraints,
            *registry.m_BroadPhaseLayer,
            *registry.m_ObjectVsBroadPhaseLayer,
            *registry.m_ObjectPairFilter
        );
        KAILUX_LOG_CHILD_CLR_MAGENTA("Physics system initialized")

        registry.allocResources();

        return registry;
    }

    BodyHandle PhysicsRegistry::createBody(const PhysicsBodyInfo &info)
    {
        auto slot = acquireSlot();
        if (slot == BodyHandle::s_InvalidIndex)
            return {};

        JPH::ShapeRefC shape;
        if (info.meshType == MeshType::Unknown)
            return {};
        if (info.meshType != MeshType::Loaded)
            shape = create_builtin_mesh_body(info.meshType, info.transform);
        else
            shape = create_loaded_mesh_body(info);

        auto motionType = static_cast<JPH::EMotionType>(info.bodyType);
        auto objectLayer = (info.bodyType == PhysicsBodyType::Static) ? layers::s_NonMoving : layers::s_Moving;

        const auto& transform = info.transform;
        JPH::BodyCreationSettings settings(
            shape,
            {transform.position.x, transform.position.y, transform.position.z},
            {transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w},
            motionType,
            objectLayer
            );
        if (info.bodyType == PhysicsBodyType::Static && info.canBecomeDynamic)
            settings.mAllowDynamicOrKinematic = true;

        m_BodyIds[slot] = m_PhysicsSystem->GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::Activate);
        return {slot};
    }

    void PhysicsRegistry::destroyBody(BodyHandle handle)
    {
        assert(handle.valid());
        auto id = m_BodyIds[handle.index];

        auto& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        if (bodyInterface.IsAdded(id))
            bodyInterface.RemoveBody(id);

        bodyInterface.DestroyBody(id);

        m_BodyIds[handle.index] = JPH::BodyID(JPH::BodyID::cInvalidBodyID);
        m_FreeSlots.push_back(handle.index);
    }

    void PhysicsRegistry::setBodyEnabled(BodyHandle handle, bool enabled)
    {
        assert(handle.valid());
        auto id = m_BodyIds[handle.index];

        auto& bodyInterface = m_PhysicsSystem->GetBodyInterface();
        bool isAdded = bodyInterface.IsAdded(id);
        if (enabled && !isAdded)
            bodyInterface.AddBody(id, JPH::EActivation::Activate);
        else if (!enabled && isAdded)
            bodyInterface.RemoveBody(id);
    }

    bool PhysicsRegistry::isBodyEnabled(BodyHandle handle) const
    {
        assert(handle.valid());
        auto id = m_BodyIds[handle.index];
        return m_PhysicsSystem->GetBodyInterface().IsAdded(id);
    }

    void PhysicsRegistry::setBodyType(BodyHandle handle, PhysicsBodyType type)
    {
        assert(handle.valid());
        auto id = m_BodyIds[handle.index];
        auto& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        auto motionType = static_cast<JPH::EMotionType>(type);
        auto layer      = (type == PhysicsBodyType::Static) ? layers::s_NonMoving : layers::s_Moving;

        auto activation = (type == PhysicsBodyType::Dynamic) ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;

        bodyInterface.SetMotionType(id, motionType, activation);

        bodyInterface.SetObjectLayer(id, layer);
    }

    void PhysicsRegistry::addForce(BodyHandle handle, const glm::vec3 &force)
    {
        assert(handle.valid());
        auto id = m_BodyIds[handle.index];
        m_PhysicsSystem->GetBodyInterface().AddForce(id, {force.x, force.y, force.z});
    }

    void PhysicsRegistry::addImpulse(BodyHandle handle, const glm::vec3 &impulse)
    {
        assert(handle.valid());
        auto id = m_BodyIds[handle.index];
        m_PhysicsSystem->GetBodyInterface().AddImpulse(id, {impulse.x, impulse.y, impulse.z});
    }

    void PhysicsRegistry::setLinearVelocity(BodyHandle handle, const glm::vec3 &velocity)
    {
        assert(handle.valid());
        auto id = m_BodyIds[handle.index];
        m_PhysicsSystem->GetBodyInterface().AddImpulse(id, {velocity.x, velocity.y, velocity.z});
    }

    glm::vec3 PhysicsRegistry::getLinearVelocity(BodyHandle handle) const
    {
        assert(handle.valid());
        auto id = m_BodyIds[handle.index];
        auto velocity = m_PhysicsSystem->GetBodyInterface().GetLinearVelocity(id);
        return {velocity.GetX(), velocity.GetY(), velocity.GetZ()};
    }

    void PhysicsRegistry::setBodyTransform(BodyHandle handle, const glm::vec3 &position, const glm::quat &rotation)
    {
        assert(handle.valid());
        auto id = m_BodyIds[handle.index];

        auto& bodyInterface = m_PhysicsSystem->GetBodyInterface();

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
        auto id = m_BodyIds[handle.index];

        JPH::BodyLockRead lock(m_PhysicsSystem->GetBodyLockInterface(), id);
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
        auto id = m_BodyIds[handle.index];

        auto& bodyInterface = m_PhysicsSystem->GetBodyInterface();
        JPH::ShapeRefC currentShape = bodyInterface.GetShape(id);
        auto baseShape = currentShape;

        if (currentShape->GetSubType() == JPH::EShapeSubType::Scaled)
        {
            const auto* scaledShape = static_cast<const JPH::ScaledShape*>(currentShape.GetPtr());
            baseShape = scaledShape->GetInnerShape();
        }

        JPH::ShapeRefC newScaledShape = new JPH::ScaledShape(
            baseShape,
            {scale.x, scale.y, scale.z}
        );
        bodyInterface.SetShape(
            id,
            newScaledShape,
            true,
            JPH::EActivation::Activate
        );
    }

    void PhysicsRegistry::update(float deltaTime)
    {
        m_PhysicsSystem->Update(
            deltaTime,
            s_CollisionSteps,
            m_Allocator.get(),
            m_JobSystem.get()
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
        m_BodyIds.resize(s_MaxBodies, JPH::BodyID(JPH::BodyID::cInvalidBodyID));

        for (uint32_t i = 0; i < s_MaxBodies; ++i)
            m_FreeSlots.push_back(i);
    }

    uint32_t PhysicsRegistry::acquireSlot()
    {
        if (m_FreeSlots.empty())
            return BodyHandle::s_InvalidIndex;
        auto slot = m_FreeSlots.front();
        m_FreeSlots.pop_front();
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
            if (info.bodyType == PhysicsBodyType::Static)
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
            // else
                // childShape = JPH::ConvexHullShapeSettings(joltVertices.data(), joltVertices.size()).Create().Get();

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
