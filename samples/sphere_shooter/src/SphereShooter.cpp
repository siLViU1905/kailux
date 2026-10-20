#include <deque>
#include <algorithm>

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

            move(camera, input, deltaTime * params.Get<float>("moveSpeed", 5.f));

            mFireCooldown = std::max(mFireCooldown - deltaTime, 0.f);
            if (input.IsMouseDown(kailux::MouseButton::Left) && mFireCooldown <= 0.f)
            {
                Shoot(camera, params.Get<float>("force", 20.f));
                mFireCooldown = 1.f / params.Get<float>("fireRate", 5.f);
            }

            const auto maxBullets{static_cast<size_t>(params.Get<int>("maxBullets", 50))};
            while (mBullets.size() > maxBullets)
            {
                GetWorld().Destroy(mBullets.front());
                mBullets.pop_front();
            }
        }

    private:
        static void move(kailux::CameraRef& camera, const kailux::Input& input, float distance)
        {
            const auto forward{glm::normalize(glm::vec3{camera.Forward().x, 0.f, camera.Forward().z})};
            const auto right{glm::normalize(glm::vec3{camera.Right().x, 0.f, camera.Right().z})};

            glm::vec3 direction{0.f};
            if (input.IsKeyDown(kailux::Key::W))
                direction += forward;
            if (input.IsKeyDown(kailux::Key::S))
                direction -= forward;
            if (input.IsKeyDown(kailux::Key::D))
                direction += right;
            if (input.IsKeyDown(kailux::Key::A))
                direction -= right;
            if (input.IsKeyDown(kailux::Key::Space))
                direction.y += 1.f;
            if (input.IsKeyDown(kailux::Key::LeftControl))
                direction.y -= 1.f;

            if (glm::dot(direction, direction) < 1e-6f)
                return;
            if (input.IsKeyDown(kailux::Key::LeftShift))
                distance *= 2.f;
            camera.SetPosition(camera.Position() + glm::normalize(direction) * distance);
        }

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
        float                      mFireCooldown{};
    };

    KAILUX_REGISTER_SCRIPT(SphereShooter);
}