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

        bool valid() const;

        bool isOpen() const;

        bool isMaximized() const;
        bool isMinimized() const;

        bool       isKeyPressed(Key key) const;
        bool       isButtonClicked(MouseButton button) const;

        glm::dvec2 getMousePos() const;
        void       setCursorMode(CursorMode mode);
        CursorMode getCursorMode() const;

        glm::ivec2 getFramebufferSize() const;

        bool operator==(const InputSource& other) const;

    private:
        GLFWwindow* mHandle{};
    };
}
