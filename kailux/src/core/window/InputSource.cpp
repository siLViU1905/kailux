#include "InputSource.h"

namespace kailux
{
    InputSource::InputSource() = default;

    InputSource::InputSource(GLFWwindow *handle) : mHandle(handle)
    {
    }

    bool InputSource::Valid() const
    {
        return mHandle != nullptr;
    }

    bool InputSource::IsOpen() const
    {
        return !glfwWindowShouldClose(mHandle);
    }

    bool InputSource::IsMaximized() const
    {
        return static_cast<bool>(glfwGetWindowAttrib(mHandle, GLFW_MAXIMIZED));
    }

    bool InputSource::IsMinimized() const
    {
        const auto size{GetFramebufferSize()};
        return !size.x || !size.y;
    }

    bool InputSource::IsKeyPressed(Key key) const
    {
        return glfwGetKey(mHandle, static_cast<int>(key)) == GLFW_PRESS;
    }

    bool InputSource::IsButtonClicked(MouseButton button) const
    {
        return glfwGetMouseButton(mHandle, static_cast<int>(button)) == GLFW_PRESS;
    }

    glm::dvec2 InputSource::GetMousePos() const
    {
        double x,y;
        glfwGetCursorPos(mHandle, &x, &y);
        return {x, y};
    }

    void InputSource::SetCursorMode(CursorMode mode)
    {
        glfwSetInputMode(mHandle, GLFW_CURSOR, static_cast<int>(mode));
    }

    CursorMode InputSource::GetCursorMode() const
    {
        return static_cast<CursorMode>(glfwGetInputMode(mHandle, GLFW_CURSOR));
    }

    glm::ivec2 InputSource::GetFramebufferSize() const
    {
        int width, height;
        glfwGetFramebufferSize(mHandle, &width, &height);
        return {width, height};
    }

    bool InputSource::operator==(const InputSource &other) const = default;
}
