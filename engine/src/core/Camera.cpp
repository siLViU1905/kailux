#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace kailux
{
    void Camera::update_movement(CameraComponent& component, const Window& window, float deltaTime)
    {
        if (!component.focused)
            return;
        
        component.right = glm::normalize(glm::cross(component.forward, component.up));

        float velocity = component.speed * deltaTime;
        if (window.getInputSource().isKeyPressed(Key::W))
            component.position += component.forward * velocity;
        if (window.getInputSource().isKeyPressed(Key::S))
            component.position -= component.forward * velocity;
        if (window.getInputSource().isKeyPressed(Key::A))
            component.position -= component.right * velocity;
        if (window.getInputSource().isKeyPressed(Key::D))
            component.position += component.right * velocity;

        if (window.getInputSource().isKeyPressed(Key::Space))
            component.position += glm::vec3(0.f, 1.f, 0.f) * velocity;
        if (window.getInputSource().isKeyPressed(Key::LeftControl))
            component.position -= glm::vec3(0.f, 1.f, 0.f) * velocity;
    }

    void Camera::update_look_at(CameraComponent& component, const Window& window, float deltaTime)
    {
        if (!component.focused)
            return;

        const auto mousePos{window.getInputSource().getMousePos()};
    
        float xOffset = static_cast<float>(mousePos.x - component.lastMousePosX);
        float yOffset = static_cast<float>(component.lastMousePosY - mousePos.y);
    
        component.lastMousePosX = mousePos.x;
        component.lastMousePosY = mousePos.y;

        component.yaw += xOffset * component.sensitivity * deltaTime;
        component.pitch += yOffset * component.sensitivity * deltaTime;
        component.pitch = clamp_pitch(component.pitch);

        update_vectors(component);
    }

    glm::mat4 Camera::get_projection(const CameraComponent& component, int width, int height)
    {
        return glm::perspective(glm::radians(component.fov),
                                static_cast<float>(width) / static_cast<float>(height),
                                component.zNear,
                                component.zFar);
    }

    glm::mat4 Camera::get_view(const CameraComponent &component)
    {
        return glm::lookAt(component.position,
                           component.position + component.forward,
                           component.up
        );
    }

    std::array<glm::vec4, 6> Camera::get_frustum_planes(const glm::mat4 &proj, const glm::mat4 &view)
    {
        std::array<glm::vec4, 6> planes;
        auto m = glm::transpose(proj * view);

        planes[0] = m[3] + m[0];
        planes[1] = m[3] - m[0];
        planes[2] = m[3] + m[1];
        planes[3] = m[3] - m[1];
        planes[4] = m[2];
        planes[5] = m[3] - m[2];

        for (auto& plane : planes)
        {
            float length = glm::length(glm::vec3(plane));
            plane /= length;
        }

        return planes;
    }

    void Camera::update_vectors(CameraComponent &component)
    {
        glm::vec3 front;
        front.x = cos(glm::radians(component.yaw)) * cos(glm::radians(component.pitch));
        front.y = sin(glm::radians(component.pitch));
        front.z = sin(glm::radians(component.yaw)) * cos(glm::radians(component.pitch));
    
        component.forward = glm::normalize(front);
        component.right = glm::normalize(glm::cross(component.forward, component.up));
    }

    float Camera::clamp_pitch(float pitch)
    {
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
        return pitch;
    }
}
