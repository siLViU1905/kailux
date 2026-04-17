#pragma once
#include "components/entt/CameraComponent.h"
#include "window/Window.h"

namespace kailux
{
    class Camera
    {
    public:
        static void updateMovement(CameraComponent& component, const Window& window, float deltaTime);
        static void updateLookAt(CameraComponent& component, const Window& window, float deltaTime);

        static glm::mat4 get_projection(const CameraComponent& component, int width, int height);
        static glm::mat4 get_view(const CameraComponent& component);

    private:
        static void update_vectors(CameraComponent& component);

        static float clamp_pitch(float pitch);
    };
}
