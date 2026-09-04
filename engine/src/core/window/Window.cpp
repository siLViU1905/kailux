#include "Window.h"

#include <stdexcept>
#include "../Log.h"

namespace kailux
{
    Window::Window() : mWindowHandle(nullptr), mFramebufferResized(false)
    {
    }

    Window::Window(Window &&other) noexcept : mWindowHandle(other.mWindowHandle),
                                              mFramebufferResized(other.mFramebufferResized),
                                              mEventQueue(std::move(other.mEventQueue))
    {
        other.mWindowHandle = nullptr;
        other.mFramebufferResized = false;
    }

    Window &Window::operator=(Window &&other) noexcept
    {
        if (this != &other)
        {
            mWindowHandle = other.mWindowHandle;
            mFramebufferResized = other.mFramebufferResized;
            mEventQueue = std::move(other.mEventQueue);

            other.mWindowHandle = nullptr;
            other.mFramebufferResized = false;
        }
        return *this;
    }

    Window::~Window()
    {
        if (mWindowHandle)
        {
            glfwDestroyWindow(mWindowHandle);
            glfwTerminate();
        }
    }

    GLFWwindow *Window::getGLFWWindow()
    {
        return mWindowHandle;
    }

    InputSource Window::getInputSource()
    {
        return {mWindowHandle};
    }

    InputSource Window::getInputSource() const
    {
        return {mWindowHandle};
    }

    void Window::initGLFW()
    {
        if (!glfwInit())
            throw std::runtime_error("Failed to init GLFW");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    }

    void Window::createWindow(int width, int height, std::string_view title)
    {
        mWindowHandle = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
        if (!mWindowHandle)
        {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }
    }

    void Window::setCallbacks() const
    {
        glfwSetFramebufferSizeCallback(mWindowHandle, glfw_framebuffer_callback);
        glfwSetKeyCallback(mWindowHandle, glfw_key_callback);
        glfwSetMouseButtonCallback(mWindowHandle, glfw_button_callback);
    }

    Window Window::create(int width, int height, std::string_view title)
    {
        log::console.debug("window: creating");
        Window window;

        window.initGLFW();
        log::console.debug("window: glfw initialized");

        window.createWindow(width, height, title);
        log::console.debug("window: window created");

        window.setCallbacks();
        log::console.debug("window: callbacks set");

        return window;
    }

    void Window::updateUserPointer()
    {
        glfwSetWindowUserPointer(mWindowHandle, this);
    }

    void Window::close()
    {
        glfwSetWindowShouldClose(mWindowHandle, true);
    }

    bool Window::wasResized()
    {
        bool temp = mFramebufferResized;

        mFramebufferResized = false;

        return temp;
    }

    void Window::maximize()
    {
        glfwMaximizeWindow(mWindowHandle);
    }

    void Window::restore()
    {
        glfwRestoreWindow(mWindowHandle);
    }

    void Window::resize(int width, int height)
    {
        glfwSetWindowSize(mWindowHandle, width, height);
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
        if (!mEventQueue.empty())
        {
            auto event = mEventQueue.front();
            mEventQueue.pop();
            return {event};
        }
        return std::nullopt;
    }

    void Window::glfw_framebuffer_callback(GLFWwindow *window, int width, int height)
    {
        auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
        self->mFramebufferResized = true;
    }

    void Window::glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));

        Key kKey = static_cast<Key>(key);
        KeyAction kAct = static_cast<KeyAction>(action);
        KeyMods kMods = static_cast<KeyMods>(mods);

        switch (kAct)
        {
            case KeyAction::Release:
                self->mEventQueue.push(KeyReleased(kKey, scancode, kMods));
                break;
            case KeyAction::Press:
                self->mEventQueue.push(KeyPressed(kKey, scancode, kMods));
                break;
            case KeyAction::Repeat:
                self->mEventQueue.push(KeyRepeated(kKey, scancode, kMods));
                break;
            default: ;
        }
    }

    void Window::glfw_button_callback(GLFWwindow *window, int button, int action, int mods)
    {
        auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));

        MouseButton mBtn = static_cast<MouseButton>(button);
        MouseAction mAct = static_cast<MouseAction>(action);
        MouseMods mMods = static_cast<MouseMods>(mods);

        switch (mAct)
        {
            case MouseAction::Release:
                self->mEventQueue.push(ButtonReleased(mBtn, mMods));
                break;
            case MouseAction::Press:
                self->mEventQueue.push(ButtonPressed(mBtn, mMods));
                break;
            default: ;
        }
    }
}
