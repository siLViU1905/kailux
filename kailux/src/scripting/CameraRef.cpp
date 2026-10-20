#include "CameraRef.h"

#include "Camera.h"
#include "components/entt/CameraComponent.h"
#include "scene/Scene.h"

namespace kailux
{
    namespace details
    {
        constexpr CameraComponent kNoCamera{};
    }

    CameraRef::CameraRef() = default;
    CameraRef::CameraRef(Scene &scene, entt::entity handle, glm::ivec2 viewportSize) : pScene(&scene),
                                                                                       mHandle(handle),
                                                                                       mViewportSize(viewportSize)
    {
    }

    bool CameraRef::Valid() const
    {
        return pScene != nullptr && pScene->GetEntityRegistry().all_of<CameraComponent>(mHandle);
    }

    CameraRef::operator bool() const
    {
        return Valid();
    }

    Entity CameraRef::GetEntity() const
    {
        return Valid() ? Entity{*pScene, mHandle} : Entity{};
    }

    glm::vec3 CameraRef::Position() const
    {
        return Valid() ? pScene->GetEntityRegistry().get<CameraComponent>(mHandle).position : glm::vec3{0.f};
    }

    void CameraRef::SetPosition(const glm::vec3 &position)
    {
        if (!Valid())
            return;
        pScene->GetEntityRegistry().get<CameraComponent>(mHandle).position = position;
        pScene->SyncCameraTransform(mHandle);
    }

    glm::vec3 CameraRef::Forward() const
    {
        return Valid() ? pScene->GetEntityRegistry().get<CameraComponent>(mHandle).forward : details::kNoCamera.forward;
    }

    glm::vec3 CameraRef::Right() const
    {
        return glm::normalize(glm::cross(Forward(), glm::vec3{0.f, 1.f, 0.f}));
    }

    glm::vec3 CameraRef::Up() const
    {
        return glm::normalize(glm::cross(Right(), Forward()));
    }

    float CameraRef::Yaw() const
    {
        return Valid() ? pScene->GetEntityRegistry().get<CameraComponent>(mHandle).yaw : details::kNoCamera.yaw;
    }

    float CameraRef::Pitch() const
    {
        return Valid() ? pScene->GetEntityRegistry().get<CameraComponent>(mHandle).pitch : details::kNoCamera.pitch;
    }

    void CameraRef::SetYawPitch(float yaw, float pitch)
    {
        if (Valid())
            camera::set_orientation(pScene->GetEntityRegistry().get<CameraComponent>(mHandle), yaw, pitch);
    }

    void CameraRef::Rotate(float yawDelta, float pitchDelta)
    {
        SetYawPitch(Yaw() + yawDelta, Pitch() + pitchDelta);
    }

    void CameraRef::LookAt(const glm::vec3 &target)
    {
        const auto offset{target - Position()};
        if (glm::length(offset) < 1e-6f)
            return;

        const auto direction{glm::normalize(offset)};
        SetYawPitch(
            glm::degrees(std::atan2(direction.z, direction.x)),
            glm::degrees(std::asin(glm::clamp(direction.y, -1.f, 1.f)))
        );
    }

    float CameraRef::Fov() const
    {
        return Valid() ? pScene->GetEntityRegistry().get<CameraComponent>(mHandle).fov : details::kNoCamera.fov;
    }

    void CameraRef::SetFov(float fov)
    {
        if (Valid())
            pScene->GetEntityRegistry().get<CameraComponent>(mHandle).fov = glm::clamp(fov, 1.f, 179.f);
    }

    Ray CameraRef::ScreenPointToRay(const glm::vec2 &screenPosition) const
    {
        if (mViewportSize.x <= 0 || mViewportSize.y <= 0)
            return CenterRay();

        const glm::vec2 size{mViewportSize};
        const glm::vec2 ndc{
            2.f * screenPosition.x / size.x - 1.f,
            1.f - 2.f * screenPosition.y / size.y
        };

        const auto tanHalfFov{std::tan(glm::radians(Fov()) * 0.5f)};
        const auto aspect{size.x / size.y};
        const auto direction{
            Forward()
            + Right() * (ndc.x * tanHalfFov * aspect)
            + Up() * (ndc.y * tanHalfFov)
        };
        return {Position(), glm::normalize(direction)};
    }

    Ray CameraRef::CenterRay() const
    {
        return {Position(), Forward()};
    }
}