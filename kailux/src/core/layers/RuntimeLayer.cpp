#include "RuntimeLayer.h"

#include "core/Log.h"

namespace kailux
{
    RuntimeLayer::RuntimeLayer(Application &application, const std::filesystem::path &scenePath) : mApplication(application),
                                                                                            mEngine(application.GetEngine()),
                                                                                            mWindow(application.GetWindow()),
                                                                                            mScenePath(scenePath)
    {
    }

    void RuntimeLayer::OnAttach()
    {
        if (!mApplication.LoadScene(mScenePath))
        {
            log::console.Error("Could not load scene '{}', see the log file", mScenePath.generic_string());
            mApplication.Close();
            return;
        }

        mStartPending = true;
    }

    void RuntimeLayer::OnUpdate(float deltaTime)
    {
        ControlPrimaryCamera();

        if (mStartPending)
        {
            mStartPending = false;
            mRunning = mEngine.RequestSimulationState(mApplication.GetSceneManager().GetActiveScene(), SimulationState::Running);
        }
    }

    bool RuntimeLayer::OnEvent(const Event &event)
    {
        if (const auto* pressed{std::get_if<ButtonPressed>(&event)};
           pressed && pressed->button == MouseButton::Middle)
        {
            mEngine.ToggleMouseLook(mApplication.GetSceneManager().GetActiveScene());
            return true;
        }

        if (const auto* released{std::get_if<KeyReleased>(&event)};
            released && released->key == Key::P)
        {
            TogglePause();
            return true;
        }

        return false;
    }

    void RuntimeLayer::ControlPrimaryCamera()
    {
        if (const auto primary{mApplication.GetSceneManager().GetActiveScene().GetPrimaryCamera()}; primary != entt::null)
            mEngine.SetControlledCamera(primary, mWindow.GetInputSource());
    }

    void RuntimeLayer::TogglePause()
    {
        if (mRunning)
        {
            mEngine.RequestSimulationState(mApplication.GetSceneManager().GetActiveScene(), SimulationState::Paused);
            mRunning = false;
        }
        else
            mRunning = mEngine.RequestSimulationState(mApplication.GetSceneManager().GetActiveScene(), SimulationState::Running);
    }
}
