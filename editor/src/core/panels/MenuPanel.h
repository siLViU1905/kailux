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

        void Render(Scene &scene) override;

        using OnSceneOpen = std::move_only_function<void()>;
        void SetOnSceneOpen(OnSceneOpen&& callback);

        using OnSceneSave = std::move_only_function<void(const std::filesystem::path&)>;
        void SetOnSceneSave(OnSceneSave&& callback);

        using OnViewMenu = std::move_only_function<void()>;
        void SetOnViewMenu(OnViewMenu&& callback);

        const glm::vec3 &GetOutlineColor() const;

        void SetDeviceInfo(const DeviceInfo &info);

    private:
        void RenderProfilerWindow();
        void RenderDeviceInfo();

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
