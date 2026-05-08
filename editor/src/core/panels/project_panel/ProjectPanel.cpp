#include "ProjectPanel.h"

#include <imgui_internal.h>

namespace kailux
{
    ProjectPanel::ProjectPanel() : m_UseFullWidth(true)
    {
    }

    ProjectPanel::ProjectPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor)
        : Panel(name, position, size, backgroundColor), m_UseFullWidth(true)
    {
    }

    void ProjectPanel::render(Scene &scene)
    {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();

        ImVec2 pos(
            viewport->Pos.x + (m_Position.x * viewport->Size.x),
            viewport->Pos.y + (m_Position.y * viewport->Size.y)
        );
        ImVec2 size(
            0.f,
            m_Size.y * viewport->Size.y
        );
        if (m_UseFullWidth)
            size.x = viewport->Size.x;
        else
            size.x = m_Size.x * viewport->Size.x;

        const ImGuiWindow *window = ImGui::FindWindowByName(m_Name.c_str());
        if (window && window->Collapsed)
        {
            float titleBarHeight = ImGui::GetFrameHeight();
            pos.y = (viewport->Pos.y + viewport->Size.y) - titleBarHeight;
        }

        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, m_BackgroundColor);

        if (ImGui::Begin(m_Name.c_str(), &m_Open,
                         ImGuiWindowFlags_NoMove))
        {
            if (ImGui::BeginTabBar("ProjectPanelTabs"))
            {
                if (ImGui::BeginTabItem("Asset Browser"))
                {
                    m_AssetBrowser.render();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Console"))
                {
                    m_Console.render();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();
    }

    void ProjectPanel::useFullWidth(bool use)
    {
        m_UseFullWidth = use;
    }

    AssetBrowser &ProjectPanel::getAssetBrowser()
    {
        return m_AssetBrowser;
    }

    Console &ProjectPanel::getConsole()
    {
        return m_Console;
    }
}
