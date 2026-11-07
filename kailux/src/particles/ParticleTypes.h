#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace kailux
{
    enum class ParticleShape : uint8_t
    {
        Point,
        Box
    };

    struct ParticleEmitterSettings
    {
        ParticleShape shape{ParticleShape::Point};
        glm::vec3     boxExtents{5.f, 0.5f, 5.f}; // half extents, Box only
        bool          confineToBox{true};         // Box only: a particle that leaves the box dies

        uint32_t maxParticles{1024};
        float    rate{100.f}; // particles per second while emitting
        bool     emitting{true};

        float lifeMin{1.f};
        float lifeMax{2.f};

        glm::vec3 direction{0.f, 1.f, 0.f};
        float     spread{25.f}; // cone half angle in degrees, 180 = every direction
        float     speedMin{2.f};
        float     speedMax{4.f};

        glm::vec3 gravity{0.f, -9.81f, 0.f};
        float     drag{0.f};       // velocity lost per second, 0 = none
        float     turbulence{0.f}; // sideways wobble

        glm::vec4 startColor{1.f, 0.8f, 0.3f, 1.f};
        glm::vec4 endColor{1.f, 0.2f, 0.f, 0.f};
        float     startSize{0.15f};
        float     endSize{0.02f};

        float stretch{0.f};    // 0 = round billboard, > 0 = streak along the velocity (rain)
        bool  additive{true};  // additive (sparks, fire) or alpha blended (smoke, snow)
    };

    struct ParticleBurst
    {
        uint32_t  count{24};
        glm::vec4 color{1.f, 0.7f, 0.3f, 1.f};
        float speed{4.f};
        float size{0.08f};
    };

    namespace particle_presets 
    {
        constexpr ParticleEmitterSettings sparks()
        {
            return {};
        }

        constexpr ParticleEmitterSettings rain()
        {
            ParticleEmitterSettings settings;
            settings.shape        = ParticleShape::Box;
            settings.boxExtents   = {15.f, 10.f, 15.f};
            settings.maxParticles = 20'000;
            settings.rate         = 20'000.f;
            settings.lifeMin      = 0.8f;
            settings.lifeMax      = 1.2f;
            settings.direction    = {0.f, -1.f, 0.f};
            settings.spread       = 2.f;
            settings.speedMin     = 18.f;
            settings.speedMax     = 22.f;
            settings.gravity      = {0.f, 0.f, 0.f};
            settings.startColor   = {0.7f, 0.75f, 0.85f, 0.45f};
            settings.endColor     = {0.7f, 0.75f, 0.85f, 0.45f};
            settings.startSize    = 0.012f;
            settings.endSize      = 0.012f;
            settings.stretch      = 0.02f;
            settings.additive     = false;
            return settings;
        }

        constexpr ParticleEmitterSettings snow()
        {
            ParticleEmitterSettings settings;
            settings.shape        = ParticleShape::Box;
            settings.boxExtents   = {15.f, 8.f, 15.f};
            settings.maxParticles = 15'000;
            settings.rate         = 2'000.f;
            settings.lifeMin      = 6.f;
            settings.lifeMax      = 9.f;
            settings.direction    = {0.f, -1.f, 0.f};
            settings.spread       = 10.f;
            settings.speedMin     = 0.8f;
            settings.speedMax     = 1.4f;
            settings.gravity      = {0.f, 0.f, 0.f};
            settings.turbulence   = 1.5f;
            settings.startColor   = {1.f, 1.f, 1.f, 0.9f};
            settings.endColor     = {1.f, 1.f, 1.f, 0.9f};
            settings.startSize    = 0.04f;
            settings.endSize      = 0.04f;
            settings.additive     = false;
            return settings;
        }
    }
}