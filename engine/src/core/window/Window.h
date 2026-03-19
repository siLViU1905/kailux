#pragma once
#include <optional>
#include <queue>

#include "../Core.h"
#include <string_view>
#include <GLFW/glfw3.h>

#include "Event.h"

namespace kailux
{
    class Window
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Window)
        ~Window();

        static Window create(int width, int height, std::string_view title);
        void updateUserPointer();

        bool isOpen() const;
        void close();
        void pollEvents() const;
        void waitForEvents() const;
        std::optional<Event> getEvent();

        bool wasResized();
        void maximize();
        void restore();
        void resize(int width, int height);

        bool isMaximized() const;
        bool isMinimized() const;

        void getFramebufferSize(int& width, int& height) const;
        constexpr int getWidth()  const { return m_Width;  }
        constexpr int getHeight() const { return m_Height; }

        GLFWwindow* getGLFWWindow();

    private:
        explicit Window(GLFWwindow* handle, int width, int height);

        static void glfw_framebuffer_callback(GLFWwindow* window, int width, int height);
        static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

        GLFWwindow*       m_WindowHandle;
        int               m_Width;
        int               m_Height;
        bool              m_FramebufferResized;
        std::queue<Event> m_EventQueue;
    };
}
