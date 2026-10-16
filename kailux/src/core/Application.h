#pragma once
#include <concepts>
#include <string>

#include "core/Clock.h"
#include "core/Engine.h"
#include "core/layers/LayerStack.h"
#include "core/scene/SceneManager.h"
#include "core/utilities/ThreadDispatcher.h"
#include "core/window/Window.h"

namespace kailux
{
    struct WindowInfo
    {
        int              width{};
        int              height{};
        std::string      title{"Kailux"};
    };

    struct ApplicationSpecification
    {
        WindowInfo          window;
        uint32_t            workerThreads{2};
        EngineSpecification engine;
    };

    class Application
    {
    public:
        explicit Application(const ApplicationSpecification& specification);

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        void Run();
        void Close();

        template<std::derived_from<Layer> TLayer, typename... Args>
        TLayer& PushLayer(Args&&... args)
        {
            return static_cast<TLayer&>(mLayerStack.PushLayer(create_scoped<TLayer>(std::forward<Args>(args)...)));
        }

        template<std::derived_from<Layer> TLayer, typename... Args>
        TLayer& PushOverlay(Args&&... args)
        {
            return static_cast<TLayer&>(mLayerStack.PushOverlay(create_scoped<TLayer>(std::forward<Args>(args)...)));
        }

        bool PopLayer(const Layer& layer);
        bool PopOverlay(const Layer& layer);

        decltype(auto) GetWindow(this auto&& self)
        {
            return (self.mWindow);
        }
        decltype(auto) GetEngine(this auto&& self)
        {
            return (self.mEngine);
        }
        decltype(auto) GetThreadDispatcher(this auto&& self)
        {
            return (self.mThreadDispatcher);
        }
        decltype(auto) GetSceneManager(this auto&& self)
        {
            return (self.mSceneManager);
        }

        SceneManager::Result LoadScene(const std::filesystem::path& path);

        ~Application();

    private:
        void DispatchEvent(Scene& scene, const Event& event);
        void RenderImGui(Scene& scene);
        void CommitPendingScene();

        Window           mWindow;
        Engine           mEngine;
        SceneManager     mSceneManager;
        Clock            mClock;
        ThreadDispatcher mThreadDispatcher;
        LayerStack       mLayerStack;
    };
}
