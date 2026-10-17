#pragma once
#include "Application.h"
#include "FileDialog.h"
#include "layers/Layer.h"
#include "panels/PanelStack.h"
#include "panels/project_panel/ProjectPanel.h"
#include "panels/EntityEditorPanel.h"
#include "panels/HierarchyPanel.h"
#include "panels/MenuPanel.h"
#include "panels/SimulationPanel.h"
#include "panels/ViewportPanel.h"

namespace kailux
{
    class EditorLayer final : public Layer
    {
    public:
        explicit EditorLayer(Application& application);

        void OnAttach() override;
        void OnUpdate(float deltaTime) override;
        void OnImGuiRender(Scene& scene) override;
        bool OnEvent(const Event& event) override;

        template<std::derived_from<Panel> TPanel>
        TPanel& GetPanel() { return mPanels.GetPanel<TPanel>(); }

        template<std::derived_from<Panel> TPanel>
        const TPanel& GetPanel() const { return mPanels.GetPanel<TPanel>(); }

    private:
        static constexpr std::string_view kHierarchyPanelName{"EntitiesHierarchy"};
        static constexpr std::string_view kEntityEditorName{"EntityEditor"};
        static constexpr std::string_view kProjectPanelName{"ProjectPanel"};
        static constexpr std::string_view kViewportPanelName{"SceneViewport"};
        static constexpr std::string_view kSimulationPanelName{"Simulation"};


        static constexpr ImVec4 kPanelsBackgroundColor{0.122f, 0.122f, 0.122f, 1.f};

        static void render_dock_space();

        void AddPanels();
        void SetCallbacks();

        void PollDialogs();
        void UpdatePanelState();
        void SyncPanelsFromEngine();
        void SyncEngineFromPanels();

        void SaveScene();
        void SaveScene(const std::filesystem::path& path);
        void OpenSaveSceneDialog();

        decltype(auto) GetActiveScene(this auto&& self)
        {
            return (self.mApplication.GetSceneManager().GetActiveScene());
        }

        Application&      mApplication;
        Window&           mWindow;
        Engine&           mEngine;
        SceneManager&     mSceneManager;
        ThreadDispatcher& mThreadDispatcher;

        PanelStack        mPanels;
        bool              mSimulationWasRunning{};

        FileDialog<DialogMode::SingleFile>    mLoadSceneDialog;
        FileDialog<DialogMode::SaveFile>      mSaveSceneDialog;
        FileDialog<DialogMode::MultipleFiles> mImportFilesDialog;
        FileDialog<DialogMode::Folder>        mImportFolderDialog;
    };
}
