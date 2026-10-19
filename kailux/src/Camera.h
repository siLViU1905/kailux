#pragma once
#include "components/entt/CameraComponent.h"
#include "window/InputSource.h"

namespace kailux::camera
{
    void update_movement(CameraComponent &component, InputSource source, float deltaTime);

    void update_look_at(CameraComponent &component, InputSource source, float deltaTime);

    void set_orientation(CameraComponent &component, float yaw, float pitch);

    glm::mat4 get_projection(const CameraComponent &component, int width, int height);

    glm::mat4 get_view(const CameraComponent &component);

    std::array<glm::vec4, 6> get_frustum_planes(const glm::mat4 &proj, const glm::mat4 &view);

    namespace details
    {
        void update_vectors(CameraComponent &component);

        float clamp_pitch(float pitch);
    }
}
