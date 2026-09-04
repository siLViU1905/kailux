#pragma once
#include "../../../../engine/src/core/scene/Scene.h"

namespace kailux
{
    class Panel
    {
    public:
        Panel();
        Panel(std::string_view name, ImVec4 backgroundColor, bool open = true, bool focused = false);
        virtual ~Panel() = default;

        virtual void render(Scene& scene) = 0;

        void setName(std::string_view name);
        void setBackgroundColor(ImVec4 backgroundColor);

        void open();
        void close();
        void toggle();

        bool isOpen() const;
        bool isFocused() const;

        GLFWwindow* getPlatformWindow() const;

        bool consumeToggleMouseLook();

    protected:
        std::string  mName;
        ImVec4       mBackgroundColor{};
        bool         mOpen{true};
        bool         mFocused{};
        GLFWwindow*  mPlatformWindow{};
        bool         mToggleMouseLook{};
    };
}
