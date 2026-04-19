#include "AssetBrowserPanel.h"

namespace kailux
{
    AssetBrowserPanel::AssetBrowserPanel() : m_CurrentPath(s_DefaultPath)
    {
    }

    AssetBrowserPanel::AssetBrowserPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor)
        : Panel(name, position, size, backgroundColor),
          m_CurrentPath(s_DefaultPath)
    {
    }

    void AssetBrowserPanel::render(Scene &scene)
    {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();

        ImVec2 pos(
            viewport->Pos.x + (m_Position.x * viewport->Size.x),
            viewport->Pos.y + (m_Position.y * viewport->Size.y)
        );
        ImVec2 size(
            m_Size.x * viewport->Size.x,
            m_Size.y * viewport->Size.y
        );

        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, m_BackgroundColor);

        if (ImGui::Begin(m_Name.c_str(), &m_Open,
                         ImGuiWindowFlags_NoMove))
        {
            if (m_CurrentPath != s_DefaultPath)
                if (ImGui::Button("<- Back"))
                    m_CurrentPath = m_CurrentPath.parent_path();

            float availableWidth = ImGui::GetContentRegionAvail().x;
            float cellWidthPixels = size.x * s_RelativeCellSize;
            int columnCount = static_cast<int>(availableWidth / cellWidthPixels);
            if (columnCount < 1)
                columnCount = 1;
            if (ImGui::BeginTable("AssetBrowserTable", columnCount))
            {
                float iconSizePixels = size.x * s_RelativeIconSize;
                for (const auto& entry : std::filesystem::directory_iterator(m_CurrentPath))
                {
                    ImGui::TableNextColumn();

                    bool isDirectory = entry.is_directory();
                    if (isDirectory)
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
                    else
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.6f, 1.0f));

                    auto name = entry.path().filename().string();
                    ImGui::Button(name.c_str(), {iconSizePixels, iconSizePixels});

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        if (isDirectory)
                            m_CurrentPath /= entry.path().filename();

                    ImGui::PopStyleColor();

                    ImGui::TextWrapped("%s", name.c_str());
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();
    }
}
