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

    void ProjectPanel::render(Scene &scene)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, mBackgroundColor);

        if (ImGui::Begin(mName.c_str(), &mOpen))
        {
            if (ImGui::BeginTabBar("ProjectPanelTabs"))
            {
                if (ImGui::BeginTabItem("Asset Browser"))
                {
                    mAssetBrowser.render();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Console"))
                {
                    mConsole.render();
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
        mUseFullWidth = use;
    }

    AssetBrowser &ProjectPanel::getAssetBrowser()
    {
        return mAssetBrowser;
    }

    Console &ProjectPanel::getConsole()
    {
        return mConsole;
    }
}
