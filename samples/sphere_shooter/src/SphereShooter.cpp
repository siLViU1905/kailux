#include <deque>

#include "scripting/ScriptAPI.h"

namespace sphere_shooter
{
    class SphereShooter final : public kailux::Script
    {
    protected:
        void OnCreate() override
        {
            GetWorld().SetBuiltinCameraControl(false);
            GetInput().SetCursorMode(kailux::CursorMode::Disabled);
        }

        void OnUpdate(float deltaTime) override
        {
            auto camera{GetWorld().GetMainCamera()};
            if (!camera)
                return;

            auto& input{GetInput()};
            const auto& params{GetParams()};

            const auto look{input.MouseDelta() * params.Get<float>("sensitivity", 0.1f)};
            camera.Rotate(look.x, -look.y);

            if (input.IsMouseDown(kailux::MouseButton::Left))
                Shoot(camera, params.Get<float>("force", 20.f));

            const auto maxBullets{static_cast<size_t>(params.Get<int>("maxBullets", 50))};
            while (mBullets.size() > maxBullets)
            {
                GetWorld().Destroy(mBullets.front());
                mBullets.pop_front();
            }
        }

    private:
        void Shoot(const kailux::CameraRef& camera, float force)
        {
            kailux::Transform transform;
            transform.position = camera.Position() + camera.Forward();
            transform.scale = glm::vec3{0.3f};

            auto bullet{
                GetWorld().SpawnPrimitive(
                    "Bullet",
                    kailux::Primitive::Sphere,
                    transform,
                    kailux::BodySettings{
                        kailux::BodyType::Dynamic,
                        1.f
                    }
                )
            };

            if (!bullet)
                return;

            bullet.GetBody().AddImpulse(camera.Forward() * force);
            mBullets.push_back(bullet);
        }

        std::deque<kailux::Entity> mBullets;
    };

    KAILUX_REGISTER_SCRIPT(SphereShooter);
}