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

        bool Valid() const;

        bool IsOpen() const;

        bool IsMaximized() const;
        bool IsMinimized() const;

        bool       IsKeyPressed(Key key) const;
        bool       IsButtonClicked(MouseButton button) const;

        glm::dvec2 GetMousePos() const;
        void       SetCursorMode(CursorMode mode);
        CursorMode GetCursorMode() const;

        glm::ivec2 GetFramebufferSize() const;

        bool operator==(const InputSource& other) const;

    private:
        GLFWwindow* mHandle{};
    };
}
