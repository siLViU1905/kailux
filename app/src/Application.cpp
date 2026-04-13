#include "Application.h"
#include "panels/MenuPanel.h"

namespace kailux
{
    Application::Application()
    {
    }

    Application::Application(Application &&other) noexcept : m_Window(std::move(other.m_Window)),
                                                             m_Engine(std::move(other.m_Engine)),
                                                             m_Editor(std::move(other.m_Editor))
    {
    }

    Application &Application::operator=(Application &&other) noexcept
    {
        if (this != &other)
        {
            m_Window = std::move(other.m_Window);
            m_Engine = std::move(other.m_Engine);
            m_Editor = std::move(other.m_Editor);
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
        app.setCallbacks();
        return app;
    }

    void Application::run()
    {
        m_Engine.run(m_Window);
    }

    void Application::setCallbacks()
    {
        auto& menuPanel = static_cast<MenuPanel&>(*m_Editor.getEditorLayer().getPanels()[kailux::EditorLayer::s_MenuPanelIndex]);
        menuPanel.setOnLoadMesh([this](std::string_view path)
        {
            (void)m_Engine.loadMesh(path);
        });

        m_Engine.setOnEditorRender([this](Scene& scene)
        {
            m_Editor.render(scene);
        });
    }
}
