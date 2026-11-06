#pragma once

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "Core.h"
#include "components/math/Transform.h"
#include "mesh/MeshRegistry.h"

namespace kailux
{
    using BodyHandle = Handle;

    enum class PhysicsBodyType : uint8_t
    {
        Static = 0,
        Kinematic = 1,
        Dynamic = 2,
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
        bool            isSensor{};
    };

    struct ContactEvent
    {
        uint64_t first{};
        uint64_t second{};
        bool     sensor{};
        bool     began{};
    };

    struct RaycastResult
    {
        uint64_t  userData{};
        glm::vec3 point{0.f};
        glm::vec3 normal{0.f};
        float     distance{};
    };

    struct PhysicsBodyInfo
    {
        Shared<const PhysicsMeshData> mesh;
        MeshType                      meshType{MeshType::Unknown};
        Transform                     transform;
        PhysicsCreationOptions        options;
    };
}

