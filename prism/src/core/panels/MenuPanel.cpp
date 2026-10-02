#include "MenuPanel.h"

#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include "core/components/entt/SceneSettings.h"

namespace kailux
{
    MenuPanel::MenuPanel() : mShowProfiler(false)
    {
    }

    MenuPanel::MenuPanel(std::string_view name, ImVec4 backgroundColor)
        : Panel(name, backgroundColor), mShowProfiler(false)
    {
    }

    void MenuPanel::Render(Scene &scene)
    {
        const bool visible{ImGui::BeginMainMenuBar()};
        mFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        if (visible)
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
                const std::string_view saveLabel{scene.GetSavePath().empty() ? "Save*" : "Save"};
                if (ImGui::MenuItem(saveLabel.data(), "Ctrl+S"))
                {
                    mOnSceneSave(scene.GetSavePath());
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
                auto &settings{scene.GetEntityRegistry().get<SceneSettings>(scene.GetSettingsEntity())};
                if (ImGui::BeginMenu("Outline"))
                {
                    ImGui::ColorEdit3("Color", glm::value_ptr(settings.outlineColor));
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Geometry"))
                {
                    int sceneLod{static_cast<int>(settings.sceneMaxLod)};
                    int simulationLod{static_cast<int>(settings.simulationMaxLod)};
                    ImGui::SliderInt("Scene max lod", &sceneLod, 0, details::kMaxGeometryLods - 1);
                    ImGui::SliderInt("Simulation max lod", &simulationLod, 0, details::kMaxGeometryLods - 1);
                    settings.sceneMaxLod = sceneLod;
                    settings.simulationMaxLod = simulationLod;

                    float sceneLodError{settings.sceneLodErrorThreshold};
                    float simulationLodError{settings.simulationLodErrorThreshold};
                    ImGui::SliderFloat("Scene lod error", &sceneLodError, 0.f, 10.f);
                    ImGui::SliderFloat("Simulation lod error", &simulationLodError, 0.f, 10.f);
                    settings.sceneLodErrorThreshold = sceneLodError;
                    settings.simulationLodErrorThreshold = simulationLodError;

                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Settings"))
            {
                if (ImGui::BeginMenu("Render Scale"))
                {
                    if (ImGui::Combo("Scale", &mRenderScaleIndex, kScaleLabels.data(), kScaleLabels.size()))
                        mOnRenderScaleChange(kScaleValues[mRenderScaleIndex]);
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();

            if (mShowProfiler)
                RenderProfilerWindow();
            if (mShowDevicesInfo)
                RenderDeviceInfo();
        }
    }

    void MenuPanel::SetOnSceneOpen(OnSceneOpen &&callback)
    {
        mOnSceneOpen = std::move(callback);
    }

    void MenuPanel::SetOnSceneSave(OnSceneSave &&callback)
    {
        mOnSceneSave = std::move(callback);
    }

    void MenuPanel::SetOnViewMenu(OnViewMenu &&callback)
    {
        mOnViewMenu = std::move(callback);
    }

    void MenuPanel::SetOnRenderScaleChange(OnRenderScaleChange &&callback)
    {
        mOnRenderScaleChange = std::move(callback);
    }

    void MenuPanel::SetDeviceInfo(const DeviceInfo &info)
    {
        mDeviceInfo = info;
    }

    void MenuPanel::RenderProfilerWindow()
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

    void MenuPanel::RenderDeviceInfo()
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
