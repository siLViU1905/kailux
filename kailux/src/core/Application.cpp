#include "Application.h"

namespace kailux
{
    Application::Application(const ApplicationSpecification& specification)
    {
        const auto& windowInfo{specification.window};
        mWindow = Window::create(windowInfo.width, windowInfo.height, windowInfo.title);
        mWindow.UpdateUserPointer();
        mEngine = Engine::create(mWindow);

        ThreadDispatcher::kMaxThreads = specification.workerThreads;
        mThreadDispatcher = ThreadDispatcher::get();

        mEngine.SetOnEditorRender([this](Scene& scene)
        {
            RenderImGui(scene);
        });
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

            const auto deltaTime{mClock.GetDeltaTime<float, TimeType::Seconds>()};

            for (auto& layer : mLayerStack)
                layer->OnUpdate(deltaTime);

            mEngine.Update(deltaTime);
            mEngine.Render(mWindow);
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

    Window & Application::GetWindow()
    {
        return mWindow;
    }

    Engine & Application::GetEngine()
    {
        return mEngine;
    }

    ThreadDispatcher & Application::GetThreadDispatcher()
    {
        return *mThreadDispatcher;
    }

    Application::~Application()
    {
        mLayerStack.Clear();
    }

    void Application::DispatchEvent(const Event &event)
    {
        mEngine.OnEvent(event, mWindow);

        for (auto it{mLayerStack.rbegin()}; it != mLayerStack.rend(); ++it)
            if ((*it)->OnEvent(event))
                break;
    }

    void Application::RenderImGui(Scene &scene)
    {
        for (auto& layer : mLayerStack)
            layer->OnImGuiRender(scene);
    }
}
