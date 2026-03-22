#pragma once
#include <glm/glm.hpp>

#include "window/Window.h"

namespace kailux
{
    class Camera
    {
    public:
        Camera();

        static Camera create(int windowWidth, int windowHeight, glm::vec3 position, float speed = 2.f, float sensitivity = 1.f);

        //using raw key codes until proper key handling implemented
        void updateMovement(const Window& window, float deltaTime);
        void updateLookAt(const Window& window, float deltaTime);
        void updateProjection(int width, int height);

        void           gainFocus();
        void           loseFocus();
        constexpr bool isFocused() const {return m_Focused;}

        constexpr const glm::mat4 &getProjection() const { return m_Projection; }
        constexpr const glm::mat4 &getView() const { return m_View; }
        constexpr const glm::vec3 &getPosition() const { return m_Position; }
        void                       setSensitivity(float sensitivity);
        constexpr float            getSensitivity() const {return m_Sensitivity;}
        void                       setPitch(float pitch);
        constexpr float            getPitch() const {return m_Pitch;}

    private:
        void updatePitch();

        glm::vec3 m_Position;
        glm::vec3 m_Forward;
        glm::vec3 m_Right;
        glm::vec3 m_Up;
        glm::mat4 m_View;
        glm::mat4 m_Projection;
        float     m_Yaw;
        float     m_Pitch;
        float     m_Speed;
        double    m_LastMousePosX;
        double    m_LastMousePosY;
        bool      m_Focused;
        float     m_Sensitivity;
    };
}
