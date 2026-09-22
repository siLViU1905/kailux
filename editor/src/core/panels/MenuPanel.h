#pragma once
#include "Panel.h"

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

        using OnRenderScaleChange = std::move_only_function<void(float)>;
        void SetOnRenderScaleChange(OnRenderScaleChange&& callback);

        void SetDeviceInfo(const DeviceInfo &info);

    private:
        static constexpr std::array kScaleLabels{"100%", "75%", "50%", "25%"};
        static constexpr std::array kScaleValues{1.f, 0.75f, 0.5f, 0.25f};

        void RenderProfilerWindow();
        void RenderDeviceInfo();

        static void text_centered(std::string_view text);

        bool        mShowProfiler;
        OnSceneOpen mOnSceneOpen;
        OnSceneSave mOnSceneSave;

        OnViewMenu  mOnViewMenu;

        int mRenderScaleIndex{};
        OnRenderScaleChange mOnRenderScaleChange;

        DeviceInfo mDeviceInfo;
        bool       mShowDevicesInfo{};
    };
}
