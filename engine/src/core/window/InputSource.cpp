#include "InputSource.h"

namespace kailux
{
    InputSource::InputSource() = default;

    InputSource::InputSource(GLFWwindow *handle) : mHandle(handle)
    {
    }

    bool InputSource::valid() const
    {
        return mHandle != nullptr;
    }

    bool InputSource::isOpen() const
    {
        return !glfwWindowShouldClose(mHandle);
    }

    bool InputSource::isMaximized() const
    {
        return static_cast<bool>(glfwGetWindowAttrib(mHandle, GLFW_MAXIMIZED));
    }

    bool InputSource::isMinimized() const
    {
        const auto size{getFramebufferSize()};
        return !size.x || !size.y;
    }

    bool InputSource::isKeyPressed(Key key) const
    {
        return glfwGetKey(mHandle, static_cast<int>(key)) == GLFW_PRESS;
    }

    bool InputSource::isButtonClicked(MouseButton button) const
    {
        return glfwGetMouseButton(mHandle, static_cast<int>(button)) == GLFW_PRESS;
    }

    glm::dvec2 InputSource::getMousePos() const
    {
        double x,y;
        glfwGetCursorPos(mHandle, &x, &y);
        return {x, y};
    }

    void InputSource::setCursorMode(CursorMode mode)
    {
        glfwSetInputMode(mHandle, GLFW_CURSOR, static_cast<int>(mode));
    }

    CursorMode InputSource::getCursorMode() const
    {
        return static_cast<CursorMode>(glfwGetInputMode(mHandle, GLFW_CURSOR));
    }

    glm::ivec2 InputSource::getFramebufferSize() const
    {
        int width, height;
        glfwGetFramebufferSize(mHandle, &width, &height);
        return {width, height};
    }

    bool InputSource::operator==(const InputSource &other) const = default;
}
