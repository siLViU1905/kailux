#pragma once

#include "Entity.h"

namespace kailux
{
    class Scene;

    struct Ray
    {
        glm::vec3 origin{0.f};
        glm::vec3 direction{0.f, 0.f, -1.f};
    };

    class CameraRef
    {
    public:
        CameraRef();

        bool Valid() const;
        explicit operator bool() const;

        Entity GetEntity() const;

        glm::vec3 Position() const;
        void      SetPosition(const glm::vec3& position);

        glm::vec3 Forward() const;
        glm::vec3 Right() const;
        glm::vec3 Up() const;

        float Yaw() const;
        float Pitch() const;
        void  SetYawPitch(float yaw, float pitch);
        void  Rotate(float yawDelta, float pitchDelta);
        void  LookAt(const glm::vec3& target);

        float Fov() const;
        void  SetFov(float fov);

        Ray ScreenPointToRay(const glm::vec2& screenPosition) const;
        Ray CenterRay() const;

    private:
        CameraRef(Scene& scene, entt::entity handle, glm::ivec2 viewportSize);

        Scene*       pScene{};
        entt::entity mHandle{entt::null};
        glm::ivec2   mViewportSize{0};

        friend class World;
    };
}