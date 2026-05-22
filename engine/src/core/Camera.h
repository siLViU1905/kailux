#pragma once
#include "components/entt/CameraComponent.h"
#include "window/Window.h"

namespace kailux
{
    class Camera
    {
    public:
        static void update_movement(CameraComponent& component, const Window& window, float deltaTime);
        static void update_look_at(CameraComponent& component, const Window& window, float deltaTime);

        static glm::mat4 get_projection(const CameraComponent& component, int width, int height);
        static glm::mat4 get_view(const CameraComponent& component);

        static std::array<glm::vec4, 6> get_frustum_planes(const glm::mat4& proj, const glm::mat4& view);

    private:
        static void update_vectors(CameraComponent& component);

        static float clamp_pitch(float pitch);
    };
}
