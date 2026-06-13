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

    Application Application::create(const WindowInfo &windowInfo)
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

            pollDialogs();

            auto deltaTime = m_Clock.getDeltaTime<float, TimeType::Seconds>();
            updateEditor();

            updateEngine(deltaTime, m_Window);
            m_Engine.render(m_Window);
        }
        m_Engine.waitIdle();
    }

    void Application::setCallbacks()
    {
        auto &hierarchyPanel = m_Editor.getLayer<EditorLayer>().getLayer().getPanel<HierarchyPanel>();
        hierarchyPanel.setOnEntityDeleted([this](const auto& meshComponent, auto materialComponent, auto cacheKey)
        {
            m_Engine.unregisterMesh(meshComponent.handle, cacheKey);
            m_Engine.unregisterTextureSet(materialComponent.handle);
        });
        hierarchyPanel.setOnDragDrop([this](std::string_view path)
        {
            if (Engine::is_mesh_type_supported(path))
            {
                std::string pathStr = path.data();
                if (m_Engine.isMeshCached(pathStr))
                    m_Engine.getPendingMeshDataQueue().emplace(
                        std::move(pathStr),
                        MeshLoader::LoadData(),
                        "",
                        MeshTransformData(),
                        MeshMaterialData(),
                        MeshType::Loaded
                    );
                else
                    m_ThreadDispatcher->enqueue([this, p = pathStr]()
                    {
                        if (auto data = MeshLoader::load(p))
                            m_Engine.getPendingMeshDataQueue().emplace(
                                std::move(p),
                                std::move(*data),
                                "",
                                MeshTransformData(),
                                MeshMaterialData(),
                                MeshType::Loaded
                            );
                    });
            }
        });
        hierarchyPanel.setOnNewMesh([this](auto type)
        {
            m_Engine.getPendingMeshDataQueue().emplace(
                                "",
                                MeshLoader::LoadData(),
                                "",
                                MeshTransformData(),
                                MeshMaterialData(),
                                type
                            );
        });

        auto &menuPanel = m_Editor.getLayer<EditorLayer>().getLayer().getPanel<MenuPanel>();
        menuPanel.setOnSceneSave([this]()
        {
            m_Engine.saveScene(AssetBrowser::s_DefaultPath);
        });
        menuPanel.setOnSceneOpen([this]()
        {
            m_LoadSceneDialog.open("Choose a scene", {"Kailux Scene", "*.klx"});
        });

        auto &projectPanel = m_Editor.getLayer<EditorLayer>().getLayer().getPanel<ProjectPanel>();
        projectPanel.getAssetBrowser().setOnImportFiles([this]()
        {
            m_ImportFilesDialog.open("Choose what to copy to the workspace");
        });
        projectPanel.getAssetBrowser().setOnImportFolder([this]()
        {
            m_ImportFolderDialog.open("Choose what to copy to the workspace");
        });

        m_Engine.setOnInfoLog([&projectPanel](auto message)
        {
            projectPanel.getConsole().log<LogSeverity::Info>(message);
        });
        m_Engine.setOnWarningLog([&projectPanel](auto message)
        {
            projectPanel.getConsole().log<LogSeverity::Warning>(message);
        });
        m_Engine.setOnErrorLog([&projectPanel](auto message)
        {
            projectPanel.getConsole().log<LogSeverity::Error>(message);
        });

        auto& entityEditor = m_Editor.getLayer<EditorLayer>().getLayer().getPanel<EntityEditorPanel>();
        entityEditor.setOnBodyTypeChange([this](auto component, auto type)
        {
            m_Engine.updateBodyType(component.handle, type);
        });
        entityEditor.setOnBodyScaleChange([this](auto component, const auto& scale)
        {
            m_Engine.updateBodyScale(component.handle, scale);
        });

        auto& viewportPanel = m_Editor.getLayer<EditorLayer>().getLayer().getPanel<ViewportPanel>();
        viewportPanel.setSceneTextureId(m_Engine.getSceneTextureId());

        viewportPanel.setOnClick([this, &hierarchyPanel, &entityEditor]()
        {
            if (entityEditor.isGizmoInUse())
                return;
            auto entity = static_cast<entt::entity>(m_Engine.getPickedEntity());
            hierarchyPanel.selectEntity(entity);
        });
        viewportPanel.setOnSimulationStart([this]()
        {
            m_Engine.setSimulationState(SimulationState::Running);
        });
        viewportPanel.setOnSimulationPause([this]()
        {
            m_Engine.setSimulationState(SimulationState::Paused);
        });

        m_Engine.setOnEditorRender([this](Scene &scene)
        {
            m_Editor.render(scene);
        });
    }

    void Application::pollDialogs()
    {
        if (m_LoadSceneDialog.poll())
            if (auto path = m_LoadSceneDialog.tryPopPath())
                m_Engine.loadScene(*path, m_Window.getWidth(), m_Window.getHeight());

        if (m_ImportFilesDialog.poll())
            while (auto path = m_ImportFilesDialog.tryPopPath())
                m_Editor.getLayer<EditorLayer>().getLayer().getPanel<ProjectPanel>().getAssetBrowser().import(*path);

        if (m_ImportFolderDialog.poll())
            if (auto path = m_ImportFolderDialog.tryPopPath())
                m_Editor.getLayer<EditorLayer>().getLayer().getPanel<ProjectPanel>().getAssetBrowser().import(*path);
    }

    void Application::updateEditor()
    {
        m_Editor.update();
        auto& viewportPanel = m_Editor.getLayer<EditorLayer>().getLayer().getPanel<ViewportPanel>();
        viewportPanel.setSceneTextureId(m_Engine.getSceneTextureId());
    }

    void Application::updateEngine(float deltaTime, Window& window)
    {
        m_Engine.update(deltaTime, window);
        auto sceneViewportMousePos = m_Editor.getLayer<EditorLayer>().getLayer().getPanel<ViewportPanel>().getScaledMousePos();
        auto outlineColor = m_Editor.getLayer<EditorLayer>().getLayer().getPanel<MenuPanel>().getOutlineColor();
        auto selectedEntity = static_cast<uint32_t>(m_Editor.getLayer<EditorLayer>().getLayer().getPanel<HierarchyPanel>().getSelectedEntity());
        m_Engine.setOutlineInfo(outlineColor, selectedEntity);
        m_Engine.setSceneViewportMousePos(sceneViewportMousePos.x, sceneViewportMousePos.y);
    }
}
