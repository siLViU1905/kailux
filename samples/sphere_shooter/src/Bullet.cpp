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
        GetWorld().PlayOneShotAt(
            kSoundPath,
            GetSelf().GetTransform().Position(),
            std::clamp(speed / kLoudSpeed, 0.2f, 1.f)
            );
    }
}
