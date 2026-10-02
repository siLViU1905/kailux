#include "ProjectPanel.h"

#include <imgui_internal.h>

namespace kailux
{
    ProjectPanel::ProjectPanel() : mUseFullWidth(true)
    {
    }

    ProjectPanel::ProjectPanel(std::string_view name, ImVec4 backgroundColor)
        : Panel(name, backgroundColor), mUseFullWidth(true)
    {
    }

    void ProjectPanel::Render(Scene &scene)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, mBackgroundColor);

        const bool visible{ImGui::Begin(mName.c_str(), &mOpen)};
        mFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        if (visible)
        {
            if (ImGui::BeginTabBar("ProjectPanelTabs"))
            {
                if (ImGui::BeginTabItem("Asset Browser"))
                {
                    mAssetBrowser.Render();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Console"))
                {
                    mConsole.Render();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();
    }

    void ProjectPanel::UseFullWidth(bool use)
    {
        mUseFullWidth = use;
    }

    AssetBrowser &ProjectPanel::GetAssetBrowser()
    {
        return mAssetBrowser;
    }

    Console &ProjectPanel::GetConsole()
    {
        return mConsole;
    }
}
