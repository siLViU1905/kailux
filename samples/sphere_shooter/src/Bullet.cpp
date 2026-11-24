#include "Bullet.h"

namespace sphere_shooter
{
    kailux::Primitive Bullet::get_random_bullet_primitive()
    {
        std::uniform_int_distribution dist{
            static_cast<int>(kailux::Primitive::Cube),
            static_cast<int>(kailux::Primitive::Torus)
        };

        return static_cast<kailux::Primitive>(dist(rng()));
    }

    void Bullet::OnCollisionEnter(kailux::Entity other)
    {
        const auto now{GetTime().Elapsed()};
        if (now - mLastBounce < kCooldown)
            return;

        const auto speed{glm::length(GetSelf().GetBody().GetVelocity())};
        if (speed < kMinSpeed)
            return;

        mLastBounce = now;

        const auto position{GetSelf().GetTransform().Position()};
        const auto strength{std::clamp(speed / kLoudSpeed, 0.2f, 1.f)};

        GetWorld().PlayOneShotAt(kSoundPath, position, strength);

        kailux::ParticleBurst sparks;
        sparks.count = static_cast<uint32_t>(kMinSparks + strength * (kMaxSparks - kMinSparks));
        sparks.speed = kSparkSpeed * strength + 1.f;
        sparks.color = kSparkColor;
        GetWorld().EmitParticles(position, sparks);
    }

    std::mt19937 & Bullet::rng()
    {
        static std::mt19937 rng{std::random_device{}()};
        return rng;
    }
}
