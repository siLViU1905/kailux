#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace kailux
{
    Camera::Camera() : m_Position(0.f, 0.f, 0.f)
                       , m_Forward(0.0f, 0.0f, -1.0f)
                       , m_Right(glm::normalize(glm::cross(m_Forward, m_Up)))
                       , m_Up(0.f, 1.f, 0.f)
                       , m_View(glm::lookAt(m_Position, m_Position + m_Forward, m_Up))
                       , m_Projection(1.f)
                       , m_Yaw(-90.f)
                       , m_Pitch(0.f)
                       , m_Speed(0.f)
                       , m_LastMousePosX(0.0)
                       , m_LastMousePosY(0.0)
                       , m_Focused(false)
                       , m_Sensitivity(0.f)
    {
    }

    Camera Camera::create(int windowWidth, int windowHeight, glm::vec3 position, float speed, float sensitivity)
    {
        Camera camera;
        camera.m_Projection = glm::perspective(
            glm::radians(45.f),
            static_cast<float>(windowWidth) / static_cast<float>(windowHeight),
            0.1f,
            100.f);
        camera.m_Position = position;
        camera.m_Speed = speed;
        camera.m_Sensitivity = sensitivity;
        return camera;
    }

    void Camera::updateMovement(const Window& window, float deltaTime)
    {
        m_Right = glm::normalize(glm::cross(m_Forward, m_Up));

        if (window.isKeyPressed(Key::W))
            m_Position += m_Forward * deltaTime * m_Speed;
        if (window.isKeyPressed(Key::S))
            m_Position -= m_Forward * deltaTime * m_Speed;
        if (window.isKeyPressed(Key::A))
            m_Position -= m_Right * deltaTime * m_Speed;
        if (window.isKeyPressed(Key::D))
            m_Position += m_Right * deltaTime * m_Speed;
        if (window.isKeyPressed(Key::Space))
            m_Position += glm::vec3(0.f, 1.f, 0.f) * deltaTime * m_Speed;
        if (window.isKeyPressed(Key::LeftControl))
            m_Position -= glm::vec3(0.f, 1.f, 0.f) * deltaTime * m_Speed;

        m_View = glm::lookAt(m_Position,
                             m_Position + m_Forward,
                             m_Up);
    }

    void Camera::updateLookAt(const Window& window, float deltaTime)
    {
        if (!m_Focused)
            return;

        double mousePosX, mousePosY;
        window.getMousePos(mousePosX, mousePosY);
        float xOffset = static_cast<float>(mousePosX - m_LastMousePosX);
        float yOffset = static_cast<float>(m_LastMousePosY - mousePosY);
        m_Yaw += xOffset * deltaTime * m_Sensitivity;
        float pitch = m_Pitch;
        pitch += yOffset * deltaTime * m_Sensitivity;
        setPitch(pitch);

        m_Yaw = std::fmod(m_Yaw, 360.0f);
        if (m_Yaw < 0.f)
            m_Yaw += 360.f;

        m_LastMousePosX = mousePosX;
        m_LastMousePosY = mousePosY;

        glm::vec3 direction(
            std::cos(glm::radians(m_Yaw)) * std::cos(glm::radians(m_Pitch)),
            std::sin(glm::radians(m_Pitch)),
            std::sin(glm::radians(m_Yaw)) * std::cos(glm::radians(m_Pitch))
        );
        m_Forward = glm::normalize(direction);
        m_Right = glm::normalize(glm::cross(m_Forward, m_Up));

        m_View = glm::lookAt(m_Position,
                             m_Position + m_Forward,
                             m_Up);
    }

    void Camera::updateProjection(int width, int height)
    {
        m_Projection = glm::perspective(glm::radians(45.f), static_cast<float>(width) / static_cast<float>(height),
                                        0.1f,
                                        100.f);
    }

    void Camera::gainFocus()
    {
        m_Focused = true;
    }

    void Camera::loseFocus()
    {
        m_Focused = false;
    }

    void Camera::setSensitivity(float sensitivity)
    {
        m_Sensitivity = sensitivity;
    }

    void Camera::setPitch(float pitch)
    {
        m_Pitch = pitch;

        updatePitch();
    }

    void Camera::updatePitch()
    {
        if (m_Pitch > 89.0f)
            m_Pitch = 89.0f;
        if (m_Pitch < -89.0f)
            m_Pitch = -89.0f;

        glm::vec3 direction;

        direction.x = std::cos(glm::radians(m_Yaw)) * std::cos(glm::radians(m_Pitch));
        direction.y = std::sin(glm::radians(m_Pitch));
        direction.z = std::sin(glm::radians(m_Yaw)) * std::cos(glm::radians(m_Pitch));

        m_Forward = glm::normalize(direction);

        m_Right = glm::normalize(glm::cross(m_Forward, m_Up));

        m_View = glm::lookAt(m_Position,
                             m_Position + m_Forward,
                             m_Up);
    }
}
