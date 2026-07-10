#include "Application.h"

#include "core/panels/HierarchyPanel.h"
#include "core/panels/MenuPanel.h"

namespace kailux
{
    Application::Application()
    {
    }

    Application::Application(Application &&other) noexcept : mWindow(std::move(other.mWindow)),
                                                             mEngine(std::move(other.mEngine)),
                                                             mEditor(std::move(other.mEditor)),
                                                             mThreadDispatcher(std::move(other.mThreadDispatcher))
    {
    }

    Application &Application::operator=(Application &&other) noexcept
    {
        if (this != &other)
        {
            mWindow = std::move(other.mWindow);
            mEngine = std::move(other.mEngine);
            mEditor = std::move(other.mEditor);
            mThreadDispatcher = std::move(other.mThreadDispatcher);
        }
        return *this;
    }

    Application Application::create(const WindowInfo &windowInfo)
    {
        Application app;
        app.mWindow = Window::create(windowInfo.width, windowInfo.height, windowInfo.title);
        app.mWindow.updateUserPointer();
        app.mEngine = Engine::create(app.mWindow);
        app.mEditor = Editor::create(
            app.mEngine.getAssetBrowserDirectoryTextureId(),
            app.mEngine.getAssetBrowserFileTextureId()
        );
        ThreadDispatcher::kMaxThreads = kThreadCount;
        app.mThreadDispatcher = ThreadDispatcher::get();
        app.setCallbacks();
        return app;
    }

    void Application::run()
    {
        while (mWindow.isOpen())
        {
            mClock.tick();
            mWindow.pollEvents();
            if (mWindow.isMinimized())
                continue;

            pollDialogs();

            auto deltaTime = mClock.getDeltaTime<float, TimeType::Seconds>();
            updateEditor();

            updateEngine(deltaTime, mWindow);
            mEngine.render(mWindow);
        }
        mEngine.waitIdle();
    }

    void Application::setCallbacks()
    {
        auto &hierarchyPanel = mEditor.getLayer<EditorLayer>().getLayer().getPanel<HierarchyPanel>();
        hierarchyPanel.setOnEntityDeleted([this](const auto& meshComponent, auto materialComponent, auto cacheKey)
        {
            mEngine.unregisterMesh(meshComponent.handle, cacheKey);
            mEngine.unregisterMaterial(materialComponent.handle);
        });
        hierarchyPanel.setOnDragDrop([this](std::string_view path)
        {
            if (Engine::is_mesh_type_supported(path))
            {
                std::string pathStr = path.data();
                if (mEngine.isMeshCached(pathStr))
                    mEngine.getPendingMeshDataQueue().emplace(
                        std::move(pathStr),
                        MeshLoader::LoadData(),
                        "",
                        MeshTransformData(),
                        MeshMaterialData(),
                        MeshType::Loaded
                    );
                else
                    mThreadDispatcher->enqueue([this, p = pathStr]()
                    {
                        if (auto data = MeshLoader::load(p))
                            mEngine.getPendingMeshDataQueue().emplace(
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
            mEngine.getPendingMeshDataQueue().emplace(
                                "",
                                MeshLoader::LoadData(),
                                "",
                                MeshTransformData(),
                                MeshMaterialData(),
                                type
                            );
        });
        hierarchyPanel.setOnNewLight([this](auto type)
        {
            mEngine.addLightEntity(type);
        });
        hierarchyPanel.setOnAddPhysics([this](auto entity, auto bodyType, auto canBecomeDynamic)
        {
            mEngine.addPhysicsToEntity(entity, {bodyType, canBecomeDynamic});
        });

        auto &menuPanel = mEditor.getLayer<EditorLayer>().getLayer().getPanel<MenuPanel>();
        menuPanel.setOnSceneSave([this]()
        {
            mEngine.saveScene(AssetBrowser::s_DefaultPath);
        });
        menuPanel.setOnSceneOpen([this]()
        {
            mLoadSceneDialog.open("Choose a scene", {"Kailux Scene", "*.klx"});
        });

        auto &projectPanel = mEditor.getLayer<EditorLayer>().getLayer().getPanel<ProjectPanel>();
        projectPanel.getAssetBrowser().setOnImportFiles([this]()
        {
            mImportFilesDialog.open("Choose what to copy to the workspace");
        });
        projectPanel.getAssetBrowser().setOnImportFolder([this]()
        {
            mImportFolderDialog.open("Choose what to copy to the workspace");
        });

        mEngine.setOnInfoLog([&projectPanel](auto message)
        {
            projectPanel.getConsole().log<LogSeverity::Info>(message);
        });
        mEngine.setOnWarningLog([&projectPanel](auto message)
        {
            projectPanel.getConsole().log<LogSeverity::Warning>(message);
        });
        mEngine.setOnErrorLog([&projectPanel](auto message)
        {
            projectPanel.getConsole().log<LogSeverity::Error>(message);
        });

        auto& entityEditor = mEditor.getLayer<EditorLayer>().getLayer().getPanel<EntityEditorPanel>();
        entityEditor.setOnBodyTypeChange([this](auto component, auto type)
        {
            mEngine.updateBodyType(component.handle, type);
        });
        entityEditor.setOnBodyScaleChange([this](auto component, const auto& scale)
        {
            mEngine.updateBodyScale(component.handle, scale);
        });

        auto& viewportPanel = mEditor.getLayer<EditorLayer>().getLayer().getPanel<ViewportPanel>();
        viewportPanel.setSceneTextureId(mEngine.getSceneTextureId());

        viewportPanel.setOnClick([this, &hierarchyPanel, &entityEditor]()
        {
            if (entityEditor.isGizmoInUse())
                return;
            auto entity = static_cast<entt::entity>(mEngine.getPickedEntity());
            hierarchyPanel.selectEntity(entity);
        });
        viewportPanel.setOnSimulationStart([this]()
        {
            mEngine.setSimulationState(SimulationState::Running);
        });
        viewportPanel.setOnSimulationPause([this]()
        {
            mEngine.setSimulationState(SimulationState::Paused);
        });

        mEngine.setOnEditorRender([this](Scene &scene)
        {
            mEditor.render(scene);
        });
    }

    void Application::pollDialogs()
    {
        if (mLoadSceneDialog.poll())
            if (auto path = mLoadSceneDialog.tryPopPath())
                mEngine.loadScene(*path, mWindow.getWidth(), mWindow.getHeight());

        if (mImportFilesDialog.poll())
            while (auto path = mImportFilesDialog.tryPopPath())
                mEditor.getLayer<EditorLayer>().getLayer().getPanel<ProjectPanel>().getAssetBrowser().import(*path);

        if (mImportFolderDialog.poll())
            if (auto path = mImportFolderDialog.tryPopPath())
                mEditor.getLayer<EditorLayer>().getLayer().getPanel<ProjectPanel>().getAssetBrowser().import(*path);
    }

    void Application::updateEditor()
    {
        mEditor.update();
        auto& viewportPanel = mEditor.getLayer<EditorLayer>().getLayer().getPanel<ViewportPanel>();
        viewportPanel.setSceneTextureId(mEngine.getSceneTextureId());
    }

    void Application::updateEngine(float deltaTime, Window& window)
    {
        mEngine.update(deltaTime, window);
        auto sceneViewportMousePos = mEditor.getLayer<EditorLayer>().getLayer().getPanel<ViewportPanel>().getScaledMousePos();
        auto outlineColor = mEditor.getLayer<EditorLayer>().getLayer().getPanel<MenuPanel>().getOutlineColor();
        auto selectedEntity = static_cast<uint32_t>(mEditor.getLayer<EditorLayer>().getLayer().getPanel<HierarchyPanel>().getSelectedEntity());
        mEngine.setOutlineInfo(outlineColor, selectedEntity);
        mEngine.setSceneViewportMousePos(sceneViewportMousePos.x, sceneViewportMousePos.y);
    }
}
