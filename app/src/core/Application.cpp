#include "Application.h"

#include "core/panels/HierarchyPanel.h"
#include "core/panels/MenuPanel.h"
#include "core/panels/SimulationPanel.h"

namespace kailux
{
    Application::Application(const WindowInfo &windowInfo)
    {
        mWindow = Window::create(windowInfo.width, windowInfo.height, windowInfo.title);
        mWindow.UpdateUserPointer();
        mEngine = Engine::create(mWindow);
        mEditor = Editor::create(
            mEngine.GetAssetBrowserDirectoryTextureId(),
            mEngine.GetAssetBrowserFileTextureId()
        );
        ThreadDispatcher::kMaxThreads = kThreadCount;
        mThreadDispatcher = ThreadDispatcher::get();
        SetCallbacks();
    }

    void Application::Run()
    {
        while (mWindow.GetInputSource().IsOpen())
        {
            mClock.Tick();
            mWindow.PollEvents();
            if (mWindow.GetInputSource().IsMinimized())
            {
                while (mWindow.GetEvent()) {}
                mWindow.WaitForEvents();
                continue;
            }

            while (auto event = mWindow.GetEvent())
                DispatchEvent(*event);

            PollDialogs();

            auto deltaTime = mClock.GetDeltaTime<float, TimeType::Seconds>();
            UpdateEditor();

            UpdateEngine(deltaTime, mWindow);
            mEngine.Render(mWindow);
        }
        mEngine.WaitIdle();
    }

    void Application::SetCallbacks()
    {
        auto &hierarchyPanel = mEditor.GetLayer<EditorLayer>().GetPanel<HierarchyPanel>();
        hierarchyPanel.SetOnMeshDeleted([this](const auto& meshComponent, auto cacheKey)
        {
            mEngine.UnregisterMesh(meshComponent.handle, cacheKey);
        });
        hierarchyPanel.SetOnDragDrop([this](std::string_view path)
        {
            if (Engine::is_mesh_type_supported(path))
            {
                std::string pathStr = path.data();
                if (mEngine.IsMeshCached(pathStr))
                    mEngine.GetPendingMeshDataQueue().Emplace(
                        entt::null,
                        std::move(pathStr),
                        MeshLoader::LoadData(),
                        "",
                        MeshTransformData(),
                        MeshMaterialData(),
                        MeshType::Loaded
                    );
                else
                    mThreadDispatcher->Enqueue([this, p = pathStr]()
                    {
                        if (auto data = MeshLoader::load(p))
                            mEngine.GetPendingMeshDataQueue().Emplace(
                                entt::null,
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
        hierarchyPanel.SetOnNewMesh([this](auto type)
        {
            mEngine.GetPendingMeshDataQueue().Emplace(
                                entt::null,
                                "",
                                MeshLoader::LoadData(),
                                "",
                                MeshTransformData(),
                                MeshMaterialData(),
                                type
                            );
        });
        hierarchyPanel.SetOnNewLight([this](auto type)
        {
            mEngine.AddLightEntity(type);
        });
        hierarchyPanel.SetOnAddPhysics([this](auto entity, auto bodyType, auto canBecomeDynamic)
        {
            mEngine.AddPhysicsToEntity(entity, {bodyType, canBecomeDynamic});
        });

        auto &menuPanel = mEditor.GetLayer<EditorLayer>().GetPanel<MenuPanel>();
        menuPanel.SetOnSceneSave([this](const auto& path)
        {
            if (path.empty())
            {
                mSaveSceneDialog.Open(
                    "Choose where to save the scene",
                    {},
                    std::format("{}.{}", mEngine.GetScene().GetName(), Engine::kSceneFileExtension)
                );
                return;
            }
            mEngine.SaveScene(path);
        });
        menuPanel.SetOnSceneOpen([this]()
        {
            mLoadSceneDialog.Open("Choose a scene", {"Kailux Scene", "*.klx"});
        });
        menuPanel.SetDeviceInfo(mEngine.GetDeviceInfo());

        auto &projectPanel = mEditor.GetLayer<EditorLayer>().GetPanel<ProjectPanel>();
        projectPanel.GetAssetBrowser().SetOnImportFiles([this]()
        {
            mImportFilesDialog.Open("Choose what to copy to the workspace");
        });
        projectPanel.GetAssetBrowser().SetOnImportFolder([this]()
        {
            mImportFolderDialog.Open("Choose what to copy to the workspace");
        });

        mEngine.SetOnInfoLog([&projectPanel](auto message)
        {
            projectPanel.GetConsole().Log<LogSeverity::Info>(message);
        });
        mEngine.SetOnWarningLog([&projectPanel](auto message)
        {
            projectPanel.GetConsole().Log<LogSeverity::Warning>(message);
        });
        mEngine.SetOnErrorLog([&projectPanel](auto message)
        {
            projectPanel.GetConsole().Log<LogSeverity::Error>(message);
        });

        auto& entityEditor = mEditor.GetLayer<EditorLayer>().GetPanel<EntityEditorPanel>();
        entityEditor.SetOnBodyTypeChange([this](auto component, auto type)
        {
            mEngine.UpdateBodyType(component.handle, type);
        });
        entityEditor.SetOnBodyScaleChange([this](auto component, const auto& scale)
        {
            mEngine.UpdateBodyScale(component.handle, scale);
        });

        auto& viewportPanel = mEditor.GetLayer<EditorLayer>().GetPanel<ViewportPanel>();
        viewportPanel.SetSceneTextureId(mEngine.GetSceneTextureId());

        viewportPanel.SetOnClick([this, &hierarchyPanel, &entityEditor]()
        {
            if (entityEditor.IsGizmoInUse())
                return;
            auto entity = static_cast<entt::entity>(mEngine.GetPickedEntity());
            hierarchyPanel.SelectEntity(entity);
        });
        viewportPanel.SetOnSimulationStart([this]()
        {
            mEngine.SetSimulationState(SimulationState::Running);
        });
        viewportPanel.SetOnSimulationPause([this]()
        {
            mEngine.SetSimulationState(SimulationState::Paused);
        });

        mEngine.SetOnEditorRender([this](Scene &scene)
        {
            mEditor.Render(scene);
        });
    }

    void Application::PollDialogs()
    {
        if (mLoadSceneDialog.Poll())
            if (auto path = mLoadSceneDialog.TryPopPath())
                mEngine.LoadScene(*path, mWindow);

        if (mSaveSceneDialog.Poll())
            if (auto path = mSaveSceneDialog.TryPopPath())
                mEngine.SaveScene(*path);

        if (mImportFilesDialog.Poll())
            while (auto path = mImportFilesDialog.TryPopPath())
                mEditor.GetLayer<EditorLayer>().GetPanel<ProjectPanel>().GetAssetBrowser().Import(*path);

        if (mImportFolderDialog.Poll())
            if (auto path = mImportFolderDialog.TryPopPath())
                mEditor.GetLayer<EditorLayer>().GetPanel<ProjectPanel>().GetAssetBrowser().Import(*path);
    }

    void Application::UpdateEditor()
    {
        mEditor.Update();

        auto& entityEditor{mEditor.GetLayer<EditorLayer>().GetPanel<EntityEditorPanel>()};
        entityEditor.SetCameraData(mEngine.GetCameraData());

        auto& viewportPanel{mEditor.GetLayer<EditorLayer>().GetPanel<ViewportPanel>()};
        viewportPanel.SetSceneTextureId(mEngine.GetSceneTextureId());

        auto& simulationPanel{mEditor.GetLayer<EditorLayer>().GetPanel<SimulationPanel>()};
        simulationPanel.SetTextureId(mEngine.GetSimulationTextureId());
    }

    void Application::UpdateEngine(float deltaTime, const Window& window)
    {
        auto& editorLayer{mEditor.GetLayer<EditorLayer>()};
        auto& simulation{editorLayer.GetPanel<SimulationPanel>()};
        auto& viewport{editorLayer.GetPanel<ViewportPanel>()};

        mEngine.SetSimulationViewActive(simulation.IsOpen());
        mEngine.SetSimulationViewExtent(simulation.GetExtent());

        const bool simulationHasInput = simulation.IsOpen() && simulation.IsFocused();

        mEngine.SetControlledCamera(
            simulationHasInput ? mEngine.GetScene().GetSimulationCamera()
                               : mEngine.GetScene().GetSceneCamera(),
            InputSource(simulationHasInput ? simulation.GetPlatformWindow()
                                           : viewport.GetPlatformWindow())
        );

        if (simulation.ConsumeToggleMouseLook() || viewport.ConsumeToggleMouseLook())
            mEngine.ToggleMouseLook();

        mEngine.Update(deltaTime);

        const auto sceneViewportMousePos = mEditor.GetLayer<EditorLayer>().GetPanel<ViewportPanel>().GetScaledMousePos();
        const auto outlineColor = mEditor.GetLayer<EditorLayer>().GetPanel<MenuPanel>().GetOutlineColor();
        const auto selectedEntity = static_cast<uint32_t>(mEditor.GetLayer<EditorLayer>().GetPanel<HierarchyPanel>().GetSelectedEntity());
        mEngine.SetOutlineInfo(outlineColor, selectedEntity);
        mEngine.SetSceneViewportMousePos(sceneViewportMousePos.x, sceneViewportMousePos.y);
    }

    void Application::DispatchEvent(const Event &event)
    {
        mEditor.OnEvent(event);
        mEngine.OnEvent(event, mWindow);

        if (const auto* keyReleased{std::get_if<KeyReleased>(&event)})
        {
            const auto mods{keyReleased->mods};
            switch (keyReleased->key)
            {
                case Key::S:
                    if (mods == KeyMods::Control)
                    {
                        if (mEngine.GetScene().GetSavePath().empty())
                            mSaveSceneDialog.Open(
                                "Choose where to save the scene",
                                {},
                                std::format("{}.{}", mEngine.GetScene().GetName(), Engine::kSceneFileExtension)
                            );
                        else
                            mEngine.SaveScene(mEngine.GetScene().GetSavePath());
                    }
                    break;
                default:
                    break;
            }
        }
    }
}
