#include "MenuPanel.h"

#include <fstream>
#include <glm/gtc/type_ptr.hpp>

namespace kailux
{
    MenuPanel::MenuPanel() : mShowProfiler(false), mOutlineColor(1.f, 0.f, 0.f)
    {
    }

    MenuPanel::MenuPanel(std::string_view name, ImVec4 backgroundColor)
        : Panel(name, backgroundColor), mShowProfiler(false), mOutlineColor(1.f, 0.f, 0.f)
    {
    }

    void MenuPanel::render(Scene &scene)
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene"))
                {
                }
                if (ImGui::MenuItem("Open..."))
                {
                    mOnSceneOpen();
                }
                const std::string_view saveLabel{scene.getSavePath().empty() ? "Save*" : "Save"};
                if (ImGui::MenuItem(saveLabel.data(), "Ctrl+S"))
                {
                    mOnSceneSave(scene.getSavePath());
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Profiler"))
                    mShowProfiler = true;
                if (ImGui::MenuItem("Device"))
                    mShowDevicesInfo = true;
                mOnViewMenu();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Scene"))
            {
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Settings"))
            {
                if (ImGui::BeginMenu("Outline"))
                {
                    ImGui::ColorEdit3("Color", glm::value_ptr(mOutlineColor));
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();

            if (mShowProfiler)
                renderProfilerWindow();
            if (mShowDevicesInfo)
                renderDeviceInfo();
        }
    }

    void MenuPanel::setOnSceneOpen(OnSceneOpen &&callback)
    {
        mOnSceneOpen = std::move(callback);
    }

    void MenuPanel::setOnSceneSave(OnSceneSave &&callback)
    {
        mOnSceneSave = std::move(callback);
    }

    void MenuPanel::setOnViewMenu(OnViewMenu &&callback)
    {
        mOnViewMenu = std::move(callback);
    }

    const glm::vec3 &MenuPanel::getOutlineColor() const
    {
        return mOutlineColor;
    }

    void MenuPanel::setDeviceInfo(const DeviceInfo &info)
    {
        mDeviceInfo = info;
    }

    void MenuPanel::renderProfilerWindow()
    {
        ImGui::Begin("Profiler", &mShowProfiler);

        static std::array<float, 100> frameTimeHistory{};
        static int offset = 0;

        float currentFrameTime = ImGui::GetIO().DeltaTime * 1000.0f;
        frameTimeHistory[offset] = currentFrameTime;
        offset = (offset + 1) % 100;

        float average = 0.f;
        for (int n = 0; n < 100; n++)
            average += frameTimeHistory[n];
        average /= 100.f;

        auto overlay = std::format("Frametime: {}", average);

        ImGui::PlotLines("Frame Time", frameTimeHistory.data(), 100, offset, overlay.c_str(), 0.0f, 50.0f,
                         ImVec2(0, 80.0f));

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        ImGui::End();
    }

    void MenuPanel::renderDeviceInfo()
    {
        ImGui::Begin("Device", &mShowDevicesInfo);

        text_centered(mDeviceInfo.deviceName);
        ImGui::Text("Driver name: %s", mDeviceInfo.driverName.c_str());
        ImGui::Text("Driver info: %s", mDeviceInfo.driverInfo.c_str());
        ImGui::Text("VRAM: %uMB", mDeviceInfo.vramSizeMB);

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Extensions"))
            for (const auto& ext : mDeviceInfo.extensions)
                ImGui::TextUnformatted(ext.c_str());

        ImGui::End();
    }

    void MenuPanel::text_centered(std::string_view text)
    {
        float avail = ImGui::GetContentRegionAvail().x;
        float width = ImGui::CalcTextSize(text.data()).x;
        float off   = (avail - width) * 0.5f;
        if (off > 0.0f)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
        ImGui::TextUnformatted(text.data());
    }
}
