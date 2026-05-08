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
        ImGui::PushStyleColor(ImGuiCol_WindowBg, m_BackgroundColor);

        if (ImGui::Begin(m_Name.c_str(), &m_Open))
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
