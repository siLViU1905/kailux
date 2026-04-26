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
        app.m_Editor = Editor::create(
            app.m_Engine.getAssetBrowserDirectoryTextureId(),
            app.m_Engine.getAssetBrowserFileTextureId()
        );
        ThreadDispatcher::s_MaxThreads = s_ThreadCount;
        app.m_ThreadDispatcher = ThreadDispatcher::get();
        app.setCallbacks();
        return app;
    }

    void Application::run()
    {
        while (m_Window.isOpen())
        {
            m_Clock.tick();
            m_Window.pollEvents();
            if (m_Window.isMinimized())
                continue;

            auto deltaTime = m_Clock.getDeltaTime<float, TimeType::Seconds>();
            m_Editor.update();

            m_Engine.update(deltaTime, m_Window);
            m_Engine.render(m_Window);
        }
        m_Engine.waitIdle();
    }

    void Application::setCallbacks()
    {
        auto &hierarchyPanel = m_Editor.getLayer<EditorLayer>().getLayer().getPanel<HierarchyPanel>();
        hierarchyPanel.setOnEntityDeleted([this](auto meshComponent, auto materialComponent)
        {
            m_Engine.unregisterMesh(meshComponent.handle, meshComponent.path);
            m_Engine.unregisterTextureSet(materialComponent.handle);
        });
        hierarchyPanel.setOnDragDrop([this](std::string_view path)
        {
            if (Engine::is_mesh_type_supported(path))
            {
                std::string pathStr = path.data();
                if (m_Engine.isMeshCached(pathStr))
                    m_Engine.getPendingMeshDataQueue().emplace(std::move(pathStr), MeshLoader::LoadData());
                else
                    m_ThreadDispatcher->enqueue([this, p = pathStr]()
                    {
                        if (auto data = MeshLoader::load(p))
                            m_Engine.getPendingMeshDataQueue().emplace(std::move(p), std::move(*data));
                    });
            }
        });

        auto& menuPanel = m_Editor.getLayer<EditorLayer>().getLayer().getPanel<MenuPanel>();
        menuPanel.setOnSceneSave([this]()
        {
            m_Engine.saveScene(AssetBrowserPanel::s_DefaultPath);
        });

        m_Engine.setOnEditorRender([this](Scene &scene)
        {
            m_Editor.render(scene);
        });
    }
}
