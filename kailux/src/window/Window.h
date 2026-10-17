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
        void UpdateUserPointer();

        void Close();
        void PollEvents() const;
        void WaitForEvents() const;
        std::optional<Event> GetEvent();

        bool WasResized();
        void Maximize();
        void Restore();
        void Resize(int width, int height);

        GLFWwindow* GetGlfwWindow();

        InputSource GetInputSource();
        InputSource GetInputSource() const;

    private:
        void InitGlfw();
        void CreateWindow(int width, int height, std::string_view title);
        void SetCallbacks() const;

        static void glfw_framebuffer_callback(GLFWwindow* window, int width, int height);
        static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void glfw_button_callback(GLFWwindow* window, int button, int action, int mods);

        GLFWwindow*       mWindowHandle;
        bool              mFramebufferResized;
        std::queue<Event> mEventQueue;
    };
}
