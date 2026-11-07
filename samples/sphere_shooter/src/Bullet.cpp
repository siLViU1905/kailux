#include "Bullet.h"

namespace sphere_shooter
{
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
}
