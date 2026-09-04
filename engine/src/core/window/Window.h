#pragma once
#include <optional>
#include <queue>

#include "../Core.h"
#include <string_view>
#include "InputSource.h"

namespace kailux
{
    class Window
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Window)
        ~Window();

        static Window create(int width, int height, std::string_view title);
        void updateUserPointer();

        void close();
        void pollEvents() const;
        void waitForEvents() const;
        std::optional<Event> getEvent();

        bool wasResized();
        void maximize();
        void restore();
        void resize(int width, int height);

        GLFWwindow* getGLFWWindow();

        InputSource getInputSource();
        InputSource getInputSource() const;

    private:
        void initGLFW();
        void createWindow(int width, int height, std::string_view title);
        void setCallbacks() const;

        static void glfw_framebuffer_callback(GLFWwindow* window, int width, int height);
        static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void glfw_button_callback(GLFWwindow* window, int button, int action, int mods);

        GLFWwindow*       mWindowHandle;
        bool              mFramebufferResized;
        std::queue<Event> mEventQueue;
    };
}
