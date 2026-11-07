#pragma once
#include <glm/glm.hpp>

#include "../../Core.h"

namespace kailux
{
    struct ParticleData
    {
        glm::vec4  positionAndAge{0.f};  // xyz world position, w age in seconds
        glm::vec4  velocityAndLife{0.f}; // xyz velocity, w lifetime; age >= life means the slot is free
        glm::uvec4 misc{0u};             // x tint (RGBA8), y random seed, z size scale (float bits), w unused
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(ParticleData)

    struct ParticleEmitterData
    {
        glm::vec4  positionAndShape{0.f};     // xyz world position, w shape (0 point, 1 box)
        glm::vec4  boxExtentsAndSpread{0.f};  // xyz half extents, w cos(spread)
        glm::vec4  directionAndDrag{0.f};     // xyz normalized direction, w drag
        glm::vec4  gravityAndTurbulence{0.f}; // xyz gravity, w turbulence
        glm::vec4  speedAndLife{0.f};         // x speed min, y speed max, z life min, w life max
        glm::vec4  startColor{0.f};
        glm::vec4  endColor{0.f};
        glm::vec4  sizeAndStretch{0.f};       // x start size, y end size, z stretch, w additive (0/1)
        glm::uvec4 range{0u};                 // x first particle, y capacity, z spawn count, w flags
        glm::uvec4 bursts{0u};                // x first burst, y burst count, z seed, w unused

        static constexpr uint32_t kFlagConfineToBox{1u << 0};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(ParticleEmitterData)

    struct ParticleBurstData
    {
        glm::vec4  positionAndSpeed{0.f}; // xyz world position, w speed
        glm::vec4  color{0.f};
        glm::uvec4 info{0u};              // x exclusive prefix end of spawn indices, y size (float bits)
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(ParticleBurstData)
}