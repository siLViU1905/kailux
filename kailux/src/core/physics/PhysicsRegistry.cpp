#include "PhysicsRegistry.h"

#include <glm/gtx/matrix_decompose.hpp>

#include "core/Log.h"
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <execution>

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

        log::console.Debug("physics registry: creating");
        PhysicsRegistry registry;
        registry.mAllocator = create_scoped<JPH::TempAllocatorImpl>(kAllocatorSize);

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
        log::console.Debug("physics registry: physics system initialized");

        registry.AllocResources();

        return registry;
    }

    PhysicsRegistry::BodyResult PhysicsRegistry::CreateBody(const PhysicsBodyInfo &info)
    {
        auto slot = AcquireSlot();
        if (slot == BodyHandle::kInvalidIndex)
            return std::unexpected{"No more slots"};

        JPH::ShapeRefC shape;
        if (info.meshType == MeshType::Unknown)
            return std::unexpected{"Unknown mesh type"};
        if (info.meshType != MeshType::Loaded)
        {
            const auto result{create_builtin_mesh_body(info.meshType, info.transform)};
            if (!result)
                return std::unexpected{result.error()};
            shape = *result;
        }
        else
        {
            const auto result{create_loaded_mesh_body(info)};
            if (!result)
                return std::unexpected{result.error()};
            shape = *result;
        }

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

        if (info.options.bodyType == PhysicsBodyType::Static && info.options.canBecomeDynamic)
            settings.mAllowDynamicOrKinematic = true;

        mBodyIds[slot] = mPhysicsSystem->GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::Activate);

        return BodyHandle{slot};
    }

    void PhysicsRegistry::DestroyBody(BodyHandle handle)
    {
        assert(handle.Valid());
        auto id = mBodyIds[handle.index];

        auto& bodyInterface = mPhysicsSystem->GetBodyInterface();

        if (bodyInterface.IsAdded(id))
            bodyInterface.RemoveBody(id);

        bodyInterface.DestroyBody(id);

        mBodyIds[handle.index] = JPH::BodyID(JPH::BodyID::cInvalidBodyID);
        mFreeSlots.push_back(handle.index);
    }

    void PhysicsRegistry::SetBodyEnabled(BodyHandle handle, bool enabled)
    {
        assert(handle.Valid());
        auto id = mBodyIds[handle.index];

        auto& bodyInterface = mPhysicsSystem->GetBodyInterface();
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
        return mPhysicsSystem->GetBodyInterface().IsAdded(id);
    }

    void PhysicsRegistry::SetBodyType(BodyHandle handle, PhysicsBodyType type)
    {
        assert(handle.Valid());
        auto id = mBodyIds[handle.index];
        auto& bodyInterface = mPhysicsSystem->GetBodyInterface();

        auto motionType = static_cast<JPH::EMotionType>(type);
        auto layer      = (type == PhysicsBodyType::Static) ? layers::kNonMoving : layers::kMoving;

        auto activation = (type == PhysicsBodyType::Dynamic) ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;

        bodyInterface.SetMotionType(id, motionType, activation);

        bodyInterface.SetObjectLayer(id, layer);
    }

    void PhysicsRegistry::AddForce(BodyHandle handle, const glm::vec3 &force)
    {
        assert(handle.Valid());
        auto id = mBodyIds[handle.index];
        mPhysicsSystem->GetBodyInterface().AddForce(id, {force.x, force.y, force.z});
    }

    void PhysicsRegistry::AddImpulse(BodyHandle handle, const glm::vec3 &impulse)
    {
        assert(handle.Valid());
        auto id = mBodyIds[handle.index];
        mPhysicsSystem->GetBodyInterface().AddImpulse(id, {impulse.x, impulse.y, impulse.z});
    }

    void PhysicsRegistry::SetLinearVelocity(BodyHandle handle, const glm::vec3 &velocity)
    {
        assert(handle.Valid());
        auto id = mBodyIds[handle.index];
        mPhysicsSystem->GetBodyInterface().AddImpulse(id, {velocity.x, velocity.y, velocity.z});
    }

    glm::vec3 PhysicsRegistry::GetLinearVelocity(BodyHandle handle) const
    {
        assert(handle.Valid());
        auto id = mBodyIds[handle.index];
        auto velocity = mPhysicsSystem->GetBodyInterface().GetLinearVelocity(id);
        return {velocity.GetX(), velocity.GetY(), velocity.GetZ()};
    }

    void PhysicsRegistry::SetBodyTransform(BodyHandle handle, const glm::vec3 &position, const glm::quat &rotation)
    {
        assert(handle.Valid());
        auto id = mBodyIds[handle.index];

        auto& bodyInterface = mPhysicsSystem->GetBodyInterface();

        JPH::Vec3 joltPos(position.x, position.y, position.z);

        JPH::Quat joltRot(rotation.x, rotation.y, rotation.z, rotation.w);

        auto activation = (bodyInterface.GetMotionType(id) == JPH::EMotionType::Dynamic)
                                    ? JPH::EActivation::Activate
                                    : JPH::EActivation::DontActivate;

        bodyInterface.SetPositionAndRotation(id, joltPos, joltRot, activation);
    }

    void PhysicsRegistry::GetBodyTransform(BodyHandle handle, glm::vec3 &outPosition, glm::quat &outRotation) const
    {
        assert(handle.Valid());
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

    void PhysicsRegistry::UpdateBodyScale(BodyHandle handle, const glm::vec3 &scale)
    {
        assert(handle.Valid());
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

    void PhysicsRegistry::Update(float deltaTime)
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

    bool PhysicsRegistry::is_degenerate_triangle(const JPH::Float3 &a, const JPH::Float3 &b, const JPH::Float3 &c)
    {
        const JPH::Vec3 v0{a};
        const JPH::Vec3 v1{b};
        const JPH::Vec3 v2{c};

        return (v1 - v0).Cross(v2 - v0).IsNearZero();
    }

    bool PhysicsRegistry::scale_is_usbale(const glm::vec3 &scale)
    {
        constexpr auto kEps{1e-6f};

        return std::abs(scale.x) > kEps
               && std::abs(scale.y) > kEps
               && std::abs(scale.z) > kEps;
    }

    void PhysicsRegistry::AllocResources()
    {
        mBodyIds.resize(kMaxBodies, JPH::BodyID(JPH::BodyID::cInvalidBodyID));

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

    PhysicsRegistry::CreateResult PhysicsRegistry::create_builtin_mesh_body(MeshType type, const Transform &transform)
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

    PhysicsRegistry::CreateResult PhysicsRegistry::create_loaded_mesh_body(const PhysicsBodyInfo &info)
    {
        const auto submeshesCount{info.submeshes.size()};

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

    PhysicsRegistry::BuildResult PhysicsRegistry::build_submesh_shape(const PhysicsBodyInfo &info, size_t idx, ChildShapeResult &out)
    {
        out = {};

        const auto& submesh = info.submeshes[idx];

        const auto submeshFormatString{std::format("submesh {}: ", idx)};

        if (submesh.vertices.empty())
            return std::unexpected{submeshFormatString + "Shape has no vertices"};
        if (submesh.indices.size() < 3)
            return std::unexpected{submeshFormatString + "Shape has less than 3 indices"};


        glm::vec3 lScale, lTrans, lSkew; glm::quat lRot; glm::vec4 lPersp;
        if (!glm::decompose(submesh.localTransform, lScale, lRot, lTrans, lSkew, lPersp))
            return std::unexpected{submeshFormatString + "Transform could not be decomposed"};

        if (!scale_is_usbale(lScale))
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
            joltVertices.reserve(submesh.vertices.size());
            for (const auto& vertex : submesh.vertices)
                joltVertices.emplace_back(
                    vertex.position.x,
                    vertex.position.y,
                    vertex.position.z
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
            hullVertices.reserve(submesh.vertices.size());
            for (const auto& vertex : submesh.vertices)
                hullVertices.emplace_back(
                    vertex.position.x,
                    vertex.position.y,
                    vertex.position.z
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
