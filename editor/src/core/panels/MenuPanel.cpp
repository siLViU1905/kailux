#include "MenuPanel.h"

#include <fstream>
#include <glm/gtc/type_ptr.hpp>

namespace kailux
{
    MenuPanel::MenuPanel() : m_ShowProfiler(false)
    {
    }

    MenuPanel::MenuPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor)
        : Panel(name, position, size, backgroundColor), m_ShowProfiler(false)
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
                }
                if (ImGui::MenuItem("Save"))
                {
                    m_OnSceneSave();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Profiler"))
                    m_ShowProfiler = true;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Scene"))
            {
                if (ImGui::BeginMenu("Ambient Light"))
                {
                    ImGui::ColorEdit3("Color", glm::value_ptr(scene.getAmbient()));
                    ImGui::SliderFloat("Intensity", &scene.getAmbient().w, 0.f, 1.f);

                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();

            if (m_ShowProfiler)
                renderProfilerWindow();
        }
    }

    void MenuPanel::setOnSceneSave(OnSceneSave &&callback)
    {
        m_OnSceneSave = std::move(callback);
    }

    void MenuPanel::renderProfilerWindow()
    {
        ImGui::Begin("Profiler", &m_ShowProfiler);

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
}
