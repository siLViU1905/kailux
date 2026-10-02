#pragma once
#include "core/Application.h"
#include "core/FileDialog.h"
#include "core/layers/Layer.h"
#include "core/panels/PanelStack.h"
#include "core/panels/project_panel/ProjectPanel.h"
#include "core/panels/EntityEditorPanel.h"
#include "core/panels/HierarchyPanel.h"
#include "core/panels/MenuPanel.h"
#include "core/panels/SimulationPanel.h"
#include "core/panels/ViewportPanel.h"

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
        void OpenSaveSceneDialog();

        Window&           mWindow;
        Engine&           mEngine;
        ThreadDispatcher& mThreadDispatcher;

        PanelStack        mPanels;
        bool              mSimulationWasRunning{};

        FileDialog<DialogMode::SingleFile>    mLoadSceneDialog;
        FileDialog<DialogMode::SaveFile>      mSaveSceneDialog;
        FileDialog<DialogMode::MultipleFiles> mImportFilesDialog;
        FileDialog<DialogMode::Folder>        mImportFolderDialog;
    };
}
