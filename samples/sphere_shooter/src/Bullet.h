#pragma once
#include "scripting/ScriptAPI.h"

#include <string_view>

namespace sphere_shooter
{
    class Bullet final : public kailux::Script
    {
    protected:
        void OnCollisionEnter(kailux::Entity other) override;

    private:
        static constexpr std::string_view  kSoundPath{"sounds/boing.wav"};
        static constexpr float             kMinSpeed{1.5f};
        static constexpr float             kLoudSpeed{15.f};
        static constexpr float             kCooldown{0.08f};

        static constexpr float             kMinSparks{8.f};
        static constexpr float             kMaxSparks{48.f};
        static constexpr float             kSparkSpeed{7.f};
        static constexpr glm::vec4         kSparkColor{1.f, 0.65f, 0.25f, 1.f};

        float mLastBounce{-1.f};
    };
}