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

        float mLastBounce{-1.f};
    };
}