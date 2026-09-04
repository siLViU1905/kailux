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

        virtual void Render(Scene& scene) = 0;

        void SetName(std::string_view name);
        void SetBackgroundColor(ImVec4 backgroundColor);

        void Open();
        void Close();
        void Toggle();

        bool IsOpen() const;
        bool IsFocused() const;

        GLFWwindow* GetPlatformWindow() const;

        bool ConsumeToggleMouseLook();

    protected:
        std::string  mName;
        ImVec4       mBackgroundColor{};
        bool         mOpen{true};
        bool         mFocused{};
        GLFWwindow*  mPlatformWindow{};
        bool         mToggleMouseLook{};
    };
}
