#include "Window.h"

#include <stdexcept>
#include "../Logger.h"

namespace kailux
{
    Window::Window() : m_WindowHandle(nullptr), m_Width(0), m_Height(0), m_FramebufferResized(false)
    {
    }

    GLFWwindow * Window::getGLFWWindow()
    {
        return m_WindowHandle;
    }

    Window::Window(GLFWwindow *handle, int width, int height)
        : m_WindowHandle(handle), m_Width(width), m_Height(height), m_FramebufferResized(false)
    {
    }

    Window Window::create(int width, int height, std::string_view title)
    {
        KAILUX_LOG_PARENT_CLR_BLUE("[WINDOW]")
        if (!glfwInit())
            throw std::runtime_error("Failed to init GLFW");
        KAILUX_LOG_CHILD_CLR_BLUE("GLFW initialized")

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        GLFWwindow *handle = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
        if (!handle)
        {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }
        KAILUX_LOG_CHILD_CLR_BLUE("Window created")

        Window window(handle, width, height);

        glfwSetFramebufferSizeCallback(handle, glfw_framebuffer_callback);
        glfwSetKeyCallback(handle, glfw_key_callback);

        return window;
    }

    void Window::updateUserPointer()
    {
        glfwSetWindowUserPointer(m_WindowHandle, this);
    }

    Window::Window(Window &&other) noexcept : m_WindowHandle(other.m_WindowHandle),
                                              m_FramebufferResized(other.m_FramebufferResized),
                                              m_Width(other.m_Width),
                                              m_Height(other.m_Height)
    {
        other.m_WindowHandle = nullptr;

        other.m_FramebufferResized = false;

        other.m_Width = 0;

        other.m_Height = 0;
    }

    bool Window::isOpen() const
    {
        return !glfwWindowShouldClose(m_WindowHandle);
    }

    void Window::close()
    {
        glfwSetWindowShouldClose(m_WindowHandle, true);
    }

    bool Window::wasResized()
    {
        bool temp = m_FramebufferResized;

        m_FramebufferResized = false;

        return temp;
    }

    void Window::maximize()
    {
        glfwMaximizeWindow(m_WindowHandle);
    }

    bool Window::isMaximized() const
    {
        return static_cast<bool>(glfwGetWindowAttrib(m_WindowHandle, GLFW_MAXIMIZED));
    }

    bool Window::isMinimized() const
    {
        return !m_Width || !m_Height;
    }

    void Window::restore()
    {
        glfwRestoreWindow(m_WindowHandle);
    }

    void Window::resize(int width, int height)
    {
        glfwSetWindowSize(m_WindowHandle, width, height);
    }

    void Window::getFramebufferSize(int &width, int &height) const
    {
        glfwGetFramebufferSize(m_WindowHandle, &width, &height);
    }

    void Window::pollEvents() const
    {
        glfwPollEvents();
    }

    void Window::waitForEvents() const
    {
        glfwWaitEvents();
    }

    std::optional<Event> Window::getEvent()
    {
        if (!m_EventQueue.empty())
        {
            auto event = m_EventQueue.front();
            m_EventQueue.pop();
            return {event};
        }
        return std::nullopt;
    }

    void Window::glfw_framebuffer_callback(GLFWwindow *window, int width, int height)
    {
        auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
        self->m_FramebufferResized = true;
        self->m_Width = width;
        self->m_Height = height;
    }

    void Window::glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));

        switch (action)
        {
            case GLFW_RELEASE:
                self->m_EventQueue.push(KeyReleased(key, scancode, mods));
                break;
            case GLFW_PRESS:
                self->m_EventQueue.push(KeyPressed(key, scancode, mods));
                break;
            case GLFW_REPEAT:
                self->m_EventQueue.push(KeyRepeated(key, scancode, mods));
                break;
            default: ;
        }
    }

    Window::~Window()
    {
        glfwDestroyWindow(m_WindowHandle);

        glfwTerminate();
    }
}
