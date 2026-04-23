#pragma once
#include "../../../editor/src/core/Editor.h"
#include "core/Engine.h"
#include "core/FileDialog.h"
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

        // will be removed soon, no need for it because drag and drop loading added
        void pollMeshLoad();

        Window           m_Window;
        Engine           m_Engine;
        Editor           m_Editor;

        Clock            m_Clock;

        static constexpr uint32_t s_ThreadCount = 2;
        Shared<ThreadDispatcher>  m_ThreadDispatcher;
        FileDialog                m_LoadMeshDialog;
    };
}
