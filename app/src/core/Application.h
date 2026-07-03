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

        static Application create(const WindowInfo &windowInfo);

        void run();

    private:
        void setCallbacks();

        void pollDialogs();

        void updateEditor();
        void updateEngine(float deltaTime, Window& window);

        Window           mWindow;
        Engine           mEngine;
        Editor           mEditor;

        Clock            mClock;

        static constexpr uint32_t kThreadCount = 2;
        Shared<ThreadDispatcher>  mThreadDispatcher;

        FileDialog<DialogMode::SingleFile>    mLoadSceneDialog;
        FileDialog<DialogMode::MultipleFiles> mImportFilesDialog;
        FileDialog<DialogMode::Folder>        mImportFolderDialog;
    };
}
