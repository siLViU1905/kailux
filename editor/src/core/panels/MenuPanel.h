#pragma once
#include "Panel.h"
#include "core/FileDialog.h"

namespace kailux
{
    class MenuPanel : public Panel
    {
    public:
        MenuPanel();
        MenuPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor);

        void render(Scene &scene) override;

        using OnSceneOpen = std::move_only_function<void()>;
        void setOnSceneOpen(OnSceneOpen&& callback);

        using OnSceneSave = std::move_only_function<void()>;
        void setOnSceneSave(OnSceneSave&& callback);

    private:
        void renderProfilerWindow();

        bool        m_ShowProfiler;
        OnSceneOpen m_OnSceneOpen;
        OnSceneSave m_OnSceneSave;
    };
}
