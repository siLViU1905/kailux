#pragma once
#include "Panel.h"
#include "core/FileDialog.h"

namespace kailux
{
    class MenuPanel : public Panel
    {
    public:
        MenuPanel();
        MenuPanel(std::string_view name, ImVec4 backgroundColor);

        void render(Scene &scene) override;

        using OnSceneOpen = std::move_only_function<void()>;
        void setOnSceneOpen(OnSceneOpen&& callback);

        using OnSceneSave = std::move_only_function<void()>;
        void setOnSceneSave(OnSceneSave&& callback);

        using OnViewMenu = std::move_only_function<void()>;
        void setOnViewMenu(OnViewMenu&& callback);

        const glm::vec3 &getOutlineColor() const;

        void setDeviceInfo(const DeviceInfo &info);

    private:
        void renderProfilerWindow();
        void renderDeviceInfo();

        static void text_centered(std::string_view text);

        bool        mShowProfiler;
        OnSceneOpen mOnSceneOpen;
        OnSceneSave mOnSceneSave;

        OnViewMenu  mOnViewMenu;

        glm::vec3   mOutlineColor;

        DeviceInfo mDeviceInfo;
        bool       mShowDevicesInfo{};
    };
}
