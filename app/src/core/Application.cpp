#include "Application.h"

#include "core/panels/HierarchyPanel.h"
#include "core/panels/MenuPanel.h"

namespace kailux
{
    Application::Application()
    {
    }

    Application::Application(Application &&other) noexcept : m_Window(std::move(other.m_Window)),
                                                             m_Engine(std::move(other.m_Engine)),
                                                             m_Editor(std::move(other.m_Editor)),
                                                             m_ThreadDispatcher(std::move(other.m_ThreadDispatcher))
    {
    }

    Application &Application::operator=(Application &&other) noexcept
    {
        if (this != &other)
        {
            m_Window = std::move(other.m_Window);
            m_Engine = std::move(other.m_Engine);
            m_Editor = std::move(other.m_Editor);
            m_ThreadDispatcher = std::move(other.m_ThreadDispatcher);
        }
        return *this;
    }

    Application Application::create(WindowInfo windowInfo)
    {
        Application app;
        app.m_Window = Window::create(windowInfo.width, windowInfo.height, windowInfo.title);
        app.m_Window.updateUserPointer();
        app.m_Engine = Engine::create(app.m_Window);
        app.m_Editor = Editor::create();
        ThreadDispatcher::s_MaxThreads = s_ThreadCount;
        app.m_ThreadDispatcher = ThreadDispatcher::get();
        app.setCallbacks();
        return app;
    }

    void Application::run()
    {
        while (m_Window.isOpen())
        {
            m_Window.pollEvents();
            if (m_Window.isMinimized())
                continue;

            pollMeshLoad();

            m_Engine.run(m_Window);
        }
        m_Engine.waitIdle();
    }

    void Application::setCallbacks()
    {
        auto &menuPanel = m_Editor.getLayer<EditorLayer>().getLayer().getPanel<MenuPanel>();
        menuPanel.setOnLoadMesh([this]()
        {
            m_LoadMeshDialog.open("Choose a supported mesh format");
        });
        auto &hierarchyPanel = m_Editor.getLayer<EditorLayer>().getLayer().getPanel<HierarchyPanel>();
        hierarchyPanel.setOnEntityDeleted([this](auto meshHandle, auto setHandle)
        {
            m_Engine.unregisterMesh(meshHandle);
            m_Engine.unregisterTextureSet(setHandle);
        });

        m_Engine.setOnEditorRender([this](Scene &scene)
        {
            m_Editor.render(scene);
        });
    }

    void Application::pollMeshLoad()
    {
        if (m_LoadMeshDialog.poll())
            while (auto path = m_LoadMeshDialog.tryPopPath())
                m_ThreadDispatcher->enqueue([this, p = *path]()
                {
                    if (auto data = MeshLoader::load(p))
                        m_Engine.getPendingDataQueue().push(std::move(*data));
                });
    }
}
