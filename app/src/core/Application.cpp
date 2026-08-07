#include "Application.h"

#include "core/panels/HierarchyPanel.h"
#include "core/panels/MenuPanel.h"

namespace kailux
{
    Application::Application(const WindowInfo &windowInfo)
    {
        mWindow = Window::create(windowInfo.width, windowInfo.height, windowInfo.title);
        mWindow.updateUserPointer();
        mEngine = Engine::create(mWindow);
        mEditor = Editor::create(
            mEngine.getAssetBrowserDirectoryTextureId(),
            mEngine.getAssetBrowserFileTextureId()
        );
        ThreadDispatcher::kMaxThreads = kThreadCount;
        mThreadDispatcher = ThreadDispatcher::get();
        setCallbacks();
    }

    void Application::run()
    {
        while (mWindow.isOpen())
        {
            mClock.tick();
            mWindow.pollEvents();
            if (mWindow.isMinimized())
            {
                while (mWindow.getEvent()) {}
                mWindow.waitForEvents();
                continue;
            }

            while (auto event = mWindow.getEvent())
                dispatchEvent(*event);

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
        auto &hierarchyPanel = mEditor.getLayer<EditorLayer>().getPanel<HierarchyPanel>();
        hierarchyPanel.setOnMeshDeleted([this](const auto& meshComponent, auto cacheKey)
        {
            mEngine.unregisterMesh(meshComponent.handle, cacheKey);
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

        auto &menuPanel = mEditor.getLayer<EditorLayer>().getPanel<MenuPanel>();
        menuPanel.setOnSceneSave([this](const auto& path)
        {
            if (path.empty())
            {
                mSaveSceneDialog.open(
                    "Choose where to save the scene",
                    {},
                    std::format("{}.{}", mEngine.getScene().getName(), Engine::kSceneFileExtension)
                );
                return;
            }
            mEngine.saveScene(path);
        });
        menuPanel.setOnSceneOpen([this]()
        {
            mLoadSceneDialog.open("Choose a scene", {"Kailux Scene", "*.klx"});
        });
        menuPanel.setDeviceInfo(mEngine.getDeviceInfo());

        auto &projectPanel = mEditor.getLayer<EditorLayer>().getPanel<ProjectPanel>();
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

        auto& entityEditor = mEditor.getLayer<EditorLayer>().getPanel<EntityEditorPanel>();
        entityEditor.setOnBodyTypeChange([this](auto component, auto type)
        {
            mEngine.updateBodyType(component.handle, type);
        });
        entityEditor.setOnBodyScaleChange([this](auto component, const auto& scale)
        {
            mEngine.updateBodyScale(component.handle, scale);
        });

        auto& viewportPanel = mEditor.getLayer<EditorLayer>().getPanel<ViewportPanel>();
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

        if (mSaveSceneDialog.poll())
            if (auto path = mSaveSceneDialog.tryPopPath())
                mEngine.saveScene(*path);

        if (mImportFilesDialog.poll())
            while (auto path = mImportFilesDialog.tryPopPath())
                mEditor.getLayer<EditorLayer>().getPanel<ProjectPanel>().getAssetBrowser().import(*path);

        if (mImportFolderDialog.poll())
            if (auto path = mImportFolderDialog.tryPopPath())
                mEditor.getLayer<EditorLayer>().getPanel<ProjectPanel>().getAssetBrowser().import(*path);
    }

    void Application::updateEditor()
    {
        mEditor.update();
        auto& viewportPanel = mEditor.getLayer<EditorLayer>().getPanel<ViewportPanel>();
        viewportPanel.setSceneTextureId(mEngine.getSceneTextureId());
    }

    void Application::updateEngine(float deltaTime, const Window& window)
    {
        mEngine.update(deltaTime, window);
        auto sceneViewportMousePos = mEditor.getLayer<EditorLayer>().getPanel<ViewportPanel>().getScaledMousePos();
        auto outlineColor = mEditor.getLayer<EditorLayer>().getPanel<MenuPanel>().getOutlineColor();
        auto selectedEntity = static_cast<uint32_t>(mEditor.getLayer<EditorLayer>().getPanel<HierarchyPanel>().getSelectedEntity());
        mEngine.setOutlineInfo(outlineColor, selectedEntity);
        mEngine.setSceneViewportMousePos(sceneViewportMousePos.x, sceneViewportMousePos.y);
    }

    void Application::dispatchEvent(const Event &event)
    {
        mEditor.onEvent(event);
        mEngine.onEvent(event, mWindow);

        if (const auto* keyReleased{std::get_if<KeyReleased>(&event)})
        {
            const auto mods{keyReleased->mods};
            switch (keyReleased->key)
            {
                case Key::S:
                    if (mods == KeyMods::Control)
                    {
                        if (mEngine.getScene().getSavePath().empty())
                            mSaveSceneDialog.open(
                                "Choose where to save the scene",
                                {},
                                std::format("{}.{}", mEngine.getScene().getName(), Engine::kSceneFileExtension)
                            );
                        else
                            mEngine.saveScene(mEngine.getScene().getSavePath());
                    }
                    break;
                default:
                    break;
            }
        }
    }
}
