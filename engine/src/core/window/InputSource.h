#pragma once
#include <GLFW/glfw3.h>
#include "Event.h"

namespace kailux
{
    class InputSource
    {
    public:
        InputSource();
        InputSource(GLFWwindow* handle);

        bool isOpen() const;

        bool isMaximized() const;
        bool isMinimized() const;

        bool       isKeyPressed(Key key) const;
        bool       isButtonClicked(MouseButton button) const;

        glm::dvec2 getMousePos() const;
        void       setCursorMode(CursorMode mode);
        CursorMode getCursorMode() const;

        glm::ivec2 getFramebufferSize() const;

    private:
        GLFWwindow* mHandle{};
    };
}
