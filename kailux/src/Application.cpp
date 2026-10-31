#include "Application.h"

namespace kailux
{
    Application::Application(const ApplicationSpecification& specification)
    {
        const auto& windowInfo{specification.window};
        mWindow = Window::create(windowInfo.width, windowInfo.height, windowInfo.title);
        mWindow.UpdateUserPointer();

        const auto& engineInfo{specification.engine};
        mEngine = Engine::create(mWindow, engineInfo);
        mEngine.GetPhysicsSystem().SetTickRate(engineInfo.physicsTickRate);

        mThreadDispatcher = ThreadDispatcher::create(specification.workerThreads);
        mEngine.SetThreadDispatcher(mThreadDispatcher);
        mEngine.SetOnLoadSceneRequest([this](const auto& path)
        {
            return LoadScene(path).has_value();
        });

        mSceneManager = SceneManager::create();
        mEngine.AttachScene(mSceneManager.GetActiveScene(), {});

        mEngine.SetOnImGuiRender([this](Scene& scene)
        {
            RenderImGui(scene);
        });
    }

    void Application::Run()
    {
        while (mWindow.GetInputSource().IsOpen())
        {
            mClock.Tick();
            CommitPendingScene();
            auto& scene{mSceneManager.GetActiveScene()};

            mWindow.PollEvents();
            if (mWindow.GetInputSource().IsMinimized())
            {
                while (mWindow.GetEvent()) {}
                mWindow.WaitForEvents();
                continue;
            }

            while (auto event = mWindow.GetEvent())
                DispatchEvent(scene, *event);

            mSceneCommandQueue.Flush(*this, scene);

            const auto deltaTime{mClock.GetDeltaTime<float, TimeType::Seconds>()};

            for (auto& layer : mLayerStack)
                layer->OnUpdate(deltaTime);

            mEngine.Update(scene, deltaTime);
            mEngine.Render(scene, mWindow);
        }
        mEngine.WaitIdle();
    }

    void Application::Close()
    {
        mWindow.Close();
    }

    bool Application::PopLayer(const Layer &layer)
    {
        return mLayerStack.PopLayer(layer);
    }

    bool Application::PopOverlay(const Layer &layer)
    {
        return mLayerStack.PopOverlay(layer);
    }

    SceneManager::Result Application::LoadScene(const std::filesystem::path &path)
    {
        return mSceneManager.StageFromFile(path, mEngine.GetSceneLoadContext(mWindow));
    }

    Application::~Application()
    {
        mLayerStack.Clear();
    }

    void Application::DispatchEvent(Scene& scene, const Event &event)
    {
        mEngine.OnEvent(scene, event, mWindow);

        for (auto it{mLayerStack.rbegin()}; it != mLayerStack.rend(); ++it)
            if ((*it)->OnEvent(event))
                break;
    }

    void Application::RenderImGui(Scene &scene)
    {
        for (auto& layer : mLayerStack)
            layer->OnImGuiRender(scene);
    }

    void Application::CommitPendingScene()
    {
        if (!mSceneManager.HasPending())
            return;

        mEngine.DetachScene(mSceneManager.GetActiveScene());
        const auto meshRequests{mSceneManager.CommitPending()};
        mEngine.AttachScene(mSceneManager.GetActiveScene(), *meshRequests);
        mSceneCommandQueue.Clear();
    }
}
