#pragma once
#include "Editor.h"
#include "core/Engine.h"
#include "core/window/Window.h"

namespace kailux
{
    struct WindowInfo
    {
        int              width{};
        int              height{};
        std::string_view title;
    };

    class Application
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Application)

        static Application create(WindowInfo windowInfo);

        void run();

    private:
        void setCallbacks();

        Window m_Window;
        Engine m_Engine;
        Editor m_Editor;
    };
}
