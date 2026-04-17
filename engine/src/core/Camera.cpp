#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace kailux
{
    void Camera::updateMovement(CameraComponent& component, const Window& window, float deltaTime)
    {
        component.right = glm::normalize(glm::cross(component.forward, component.up));

        float velocity = component.speed * deltaTime;
        if (window.isKeyPressed(Key::W))
            component.position += component.forward * velocity;
        if (window.isKeyPressed(Key::S))
            component.position -= component.forward * velocity;
        if (window.isKeyPressed(Key::A))
            component.position -= component.right * velocity;
        if (window.isKeyPressed(Key::D))
            component.position += component.right * velocity;

        if (window.isKeyPressed(Key::Space))
            component.position += glm::vec3(0.f, 1.f, 0.f) * velocity;
        if (window.isKeyPressed(Key::LeftControl))
            component.position -= glm::vec3(0.f, 1.f, 0.f) * velocity;
    }

    void Camera::updateLookAt(CameraComponent& component, const Window& window, float deltaTime)
    {
        if (!component.focused)
            return;

        double xpos, ypos;
        window.getMousePos(xpos, ypos);
    
        float xOffset = static_cast<float>(xpos - component.lastMousePosX);
        float yOffset = static_cast<float>(component.lastMousePosY - ypos);
    
        component.lastMousePosX = xpos;
        component.lastMousePosY = ypos;

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
