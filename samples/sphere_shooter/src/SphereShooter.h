#pragma once
#include "scripting/ScriptAPI.h"

#include <deque>

namespace sphere_shooter
{
    class SphereShooter final : public kailux::Script
    {
    protected:
        void OnCreate() override;

        void OnUpdate(float deltaTime) override;

    private:
        static void move(kailux::CameraRef &camera, const kailux::Input &input, float distance);

        void Shoot(const kailux::CameraRef &camera, float force);

        std::deque<kailux::Entity> mBullets;
        float                      mFireCooldown{};
    };
}
