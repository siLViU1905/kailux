#pragma once

namespace kailux
{
    struct CameraComponent
    {
        bool isPrimary = false;

        glm::vec3 position = {0.f, 0.f, 5.f};
        glm::vec3 forward = {0.0f, 0.0f, -1.0f};
        glm::vec3 right = {1.0f, 0.0f, 0.0f};
        glm::vec3 up = {0.f, 1.f, 0.f};

        float fov = 45.f;
        float zNear = 0.1f;
        float zFar = 100.f;

        float yaw = -90.f;
        float pitch = 0.f;
        float speed = 2.f;
        float sensitivity = 1.f;
        float exposure = 0.0001f;

        double lastMousePosX = 0.0;
        double lastMousePosY = 0.0;

        bool focused = false;
    };
}
