#pragma once
#include <filesystem>

#include "core/Application.h"
#include "core/layers/Layer.h"

namespace kailux
{
    class RuntimeLayer final : public Layer
    {
    public:
        explicit RuntimeLayer(Application& application, const std::filesystem::path &scenePath);

        void OnAttach() override;
        void OnUpdate(float deltaTime) override;
        bool OnEvent(const Event& event) override;

    private:
        void ControlPrimaryCamera();
        void TogglePause();

        Application&          mApplication;
        Engine&               mEngine;
        Window&               mWindow;
        std::filesystem::path mScenePath;
        bool                  mRunning{};
    };
}