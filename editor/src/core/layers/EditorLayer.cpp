#include "EditorLayer.h"

namespace kailux
{
    EditorLayer::EditorLayer(Application &application) : Layer("EditorLayer"),
                                                         mWindow(application.GetWindow()),
                                                         mEngine(application.GetEngine()),
                                                         mThreadDispatcher(application.GetThreadDispatcher())
    {
    }

    void EditorLayer::OnAttach()
    {
        AddPanels();
        SetCallbacks();
    }

    void EditorLayer::OnUpdate(float deltaTime)
    {
        PollDialogs();
        UpdatePanelState();
        SyncPanelsFromEngine();
        SyncEngineFromPanels();
    }

    void EditorLayer::OnImGuiRender(Scene &scene)
    {
        render_dock_space();
        mPanels.RenderPanels(scene);
    }

    bool EditorLayer::OnEvent(const Event &event)
    {
        const auto* keyReleased{std::get_if<KeyReleased>(&event)};
        if (!keyReleased)
            return false;
        switch (keyReleased->key)
        {
            case Key::Delete:
                GetPanel<HierarchyPanel>().DeleteSelectedEntity();
                return true;
            case Key::S:
                if (keyReleased->mods == KeyMods::Control)
                {
                    SaveScene();
                    return true;
                }
                return false;
            default:
                return false;
        }
    }

    void EditorLayer::render_dock_space()
    {
        const auto *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
                                                  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                                  ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavInputs;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        ImGui::Begin("KailuxDockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        auto dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.f, 0.f));

        ImGui::End();
    }

    void EditorLayer::AddPanels()
    {
        auto &viewportPanel{mPanels.EmplacePanel<ViewportPanel>(kViewportPanelName)};
        auto& menuPanel{mPanels.EmplacePanel<MenuPanel>()};
        auto &hierarchyPanel{mPanels.EmplacePanel<HierarchyPanel>(
            kHierarchyPanelName,
            kPanelsBackgroundColor)};
        auto &entityEditorPanel{mPanels.EmplacePanel<EntityEditorPanel>(kEntityEditorName,
                                                                  kPanelsBackgroundColor)};
        auto &projectPanel{mPanels.EmplacePanel<ProjectPanel>(kProjectPanelName,
                                                        kPanelsBackgroundColor)};
        auto &simulationPanel{mPanels.EmplacePanel<SimulationPanel>(kSimulationPanelName)};
        simulationPanel.Close();

        hierarchyPanel.SetOnEntitySelected([&entityEditorPanel](entt::entity entity, const Scene &scene)
        {
            entityEditorPanel.Open();
            entityEditorPanel.SetSelectedEntity(entity, scene);
        });

        projectPanel.GetAssetBrowser().SetDirectoryTextureId(mEngine.GetAssetBrowserDirectoryTextureId());
        projectPanel.GetAssetBrowser().SetFileTextureId(mEngine.GetAssetBrowserFileTextureId());

        menuPanel.SetOnViewMenu([&viewportPanel, &hierarchyPanel, &entityEditorPanel, &projectPanel]()
        {
            if (ImGui::MenuItem("Scene Viewport", nullptr, viewportPanel.IsOpen()))
                viewportPanel.Toggle();
            if (ImGui::MenuItem("Entities Hierarchy", nullptr, hierarchyPanel.IsOpen()))
                hierarchyPanel.Toggle();
            if (ImGui::MenuItem("Entity Editor", nullptr, entityEditorPanel.IsOpen()))
                entityEditorPanel.Toggle();
            if (ImGui::MenuItem("Project", nullptr, projectPanel.IsOpen()))
                projectPanel.Toggle();
        });
    }

    void EditorLayer::SetCallbacks()
    {
        auto& hierarchyPanel{GetPanel<HierarchyPanel>()};
        hierarchyPanel.SetOnMeshDeleted([this](auto entity)
        {
            mEngine.UnregisterMesh(entity);
        });
        hierarchyPanel.SetOnDragDrop([this](const auto& path)
        {
            mEngine.HandleMeshDragDrop(path, mThreadDispatcher);
        });
        hierarchyPanel.SetOnNewMesh([this](auto type)
        {
            mEngine.GetPendingMeshDataQueue().Emplace(
                entt::null,
                "",
                MeshLoader::LoadData{},
                "",
                Transform{},
                MeshMaterialData{},
                type
            );
        });
        hierarchyPanel.SetOnNewLight([this](auto type)
        {
            mEngine.AddLightEntity(type);
        });
        hierarchyPanel.SetOnNewCamera([this]()
        {
            glm::ivec2 extent{};
            const auto& simulationPanel{GetPanel<SimulationPanel>()};
            if (simulationPanel.IsOpen())
                extent = simulationPanel.GetInputSource().GetFramebufferSize();
            else
                extent = GetPanel<ViewportPanel>().GetInputSource().GetFramebufferSize();
            mEngine.AddCameraEntity(extent.x, extent.y);
        });
        hierarchyPanel.SetOnAddPhysics([this](auto entity, auto bodyType, auto canBecomeDynamic)
        {
            mEngine.AddPhysicsToEntity(entity, {bodyType, canBecomeDynamic});
        });
        auto& menuPanel{GetPanel<MenuPanel>()};
        menuPanel.SetOnSceneSave([this](const auto& path)
        {
            if (path.empty())
            {
                OpenSaveSceneDialog();
                return;
            }
            mEngine.SaveScene(path);
        });
        menuPanel.SetOnSceneOpen([this]()
        {
            mLoadSceneDialog.Open("Choose a scene", {"Kailux Scene", "*.klx"});
        });
        menuPanel.SetOnRenderScaleChange([this](float scale)
        {
            mEngine.SetRenderScale(scale);
        });
        menuPanel.SetDeviceInfo(mEngine.GetDeviceInfo());
        auto& projectPanel{GetPanel<ProjectPanel>()};
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
        auto& entityEditor{GetPanel<EntityEditorPanel>()};
        entityEditor.SetOnBodyTypeChange([this](auto component, auto type)
        {
            mEngine.UpdateBodyType(component.handle, type);
        });
        entityEditor.SetOnBodyScaleChange([this](auto component, const auto& scale)
        {
            mEngine.UpdateBodyScale(component.handle, scale);
        });

        auto& viewportPanel{GetPanel<ViewportPanel>()};
        viewportPanel.SetSceneTexture(mEngine.GetSceneTextureId(), mEngine.GetSceneViewExtent());
        viewportPanel.SetOnClick([this, &hierarchyPanel, &entityEditor]()
        {
            if (entityEditor.IsGizmoInUse())
                return;
            const auto entity{static_cast<entt::entity>(mEngine.GetPickedEntity())};
            hierarchyPanel.SelectEntity(entity);
        });
        viewportPanel.SetOnSimulationStart([this]()
        {
            return mEngine.RequestSimulationState(SimulationState::Running);
        });
        viewportPanel.SetOnSimulationPause([this]()
        {
            mEngine.RequestSimulationState(SimulationState::Paused);
        });
    }

    void EditorLayer::PollDialogs()
    {
        if (mLoadSceneDialog.Poll())
            if (const auto path{mLoadSceneDialog.TryPopPath()})
                mEngine.LoadScene(*path, mWindow);
        if (mSaveSceneDialog.Poll())
            if (const auto path{mSaveSceneDialog.TryPopPath()})
                mEngine.SaveScene(*path);
        const auto& assetBrowser{GetPanel<ProjectPanel>().GetAssetBrowser()};
        if (mImportFilesDialog.Poll())
            while (const auto path{mImportFilesDialog.TryPopPath()})
                assetBrowser.Import(*path);
        if (mImportFolderDialog.Poll())
            if (const auto path{mImportFolderDialog.TryPopPath()})
                assetBrowser.Import(*path);
    }

    void EditorLayer::UpdatePanelState()
    {
        GetPanel<ProjectPanel>().UseFullWidth(!GetPanel<EntityEditorPanel>().IsOpen());
        const auto& viewport{GetPanel<ViewportPanel>()};

        const bool isSimulationRunning{viewport.GetSimulationState() != SimulationState::Paused};

        auto& entityEditor{GetPanel<EntityEditorPanel>()};
        entityEditor.SetSimulationState(isSimulationRunning);
        isSimulationRunning ? entityEditor.Lock() : entityEditor.Unlock();

        auto& hierarchyPanel{GetPanel<HierarchyPanel>()};
        isSimulationRunning ? hierarchyPanel.Lock() : hierarchyPanel.Unlock();

        mSimulationWasRunning = isSimulationRunning;
    }

    void EditorLayer::SyncPanelsFromEngine()
    {
        GetPanel<EntityEditorPanel>().SetCameraData(mEngine.GetCameraData());
        GetPanel<ViewportPanel>().SetSceneTexture(mEngine.GetSceneTextureId(), mEngine.GetSceneViewExtent());
        GetPanel<SimulationPanel>().SetTextureId(mEngine.GetSimulationTextureId());
    }

    void EditorLayer::SyncEngineFromPanels()
    {
        auto& simulation{GetPanel<SimulationPanel>()};
        auto& viewport{GetPanel<ViewportPanel>()};
        if (viewport.GetInputSource().Valid() && viewport.IsOpen())
            mEngine.SetSceneViewExtent(viewport.GetInputSource().GetFramebufferSize());
        mEngine.SetSimulationViewActive(simulation.IsOpen());
        const auto extent{
            simulation.GetInputSource().Valid() && simulation.IsOpen()
                ? simulation.GetInputSource().GetFramebufferSize()
                : glm::ivec2{}
        };
        mEngine.SetSimulationViewExtent(extent);
        const bool simulationHasInput{simulation.IsOpen() && simulation.IsFocused()};
        mEngine.SetControlledCamera(
            simulationHasInput
                ? mEngine.GetScene().GetPrimaryCamera()
                : mEngine.GetScene().GetSceneCamera(),
            simulationHasInput
                ? simulation.GetInputSource()
                : viewport.GetInputSource()
        );
        if (simulation.ConsumeToggleMouseLook() || viewport.ConsumeToggleMouseLook())
            mEngine.ToggleMouseLook();

        const auto mousePos{viewport.GetScaledMousePos()};
        mEngine.SetSceneViewportMousePos(mousePos.x, mousePos.y);
        mEngine.SetSelectedEntity(static_cast<uint32_t>(GetPanel<HierarchyPanel>().GetSelectedEntity()));
    }

    void EditorLayer::SaveScene()
    {
        const auto& savePath{mEngine.GetScene().GetSavePath()};
        if (savePath.empty())
            OpenSaveSceneDialog();
        else
            mEngine.SaveScene(savePath);
    }

    void EditorLayer::OpenSaveSceneDialog()
    {
        mSaveSceneDialog.Open(
            "Choose where to save the scene",
            {},
            std::format("{}.{}", mEngine.GetScene().GetName(), Engine::kSceneFileExtension)
        );
    }
}
