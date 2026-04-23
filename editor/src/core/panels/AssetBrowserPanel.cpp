#include "AssetBrowserPanel.h"

#include <imgui_internal.h>

namespace kailux
{
    AssetBrowserPanel::AssetBrowserPanel() : m_CurrentPath(s_DefaultPath),
                                             m_UseFullWidth(true),
                                             m_DirectoryTextureId(0),
                                             m_FileTextureId(0)
    {
    }

    AssetBrowserPanel::AssetBrowserPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor)
        : Panel(name, position, size, backgroundColor),
          m_CurrentPath(s_DefaultPath),
          m_UseFullWidth(true),
          m_DirectoryTextureId(0),
          m_FileTextureId(0)
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
            if (m_CurrentPath != s_DefaultPath)
            {
                if (ImGui::Button("<- Back"))
                    m_CurrentPath = m_CurrentPath.parent_path();
                ImGui::SameLine();
            }

            ImGui::Text("%s", m_CurrentPath.string().c_str());

            float availableWidth = ImGui::GetContentRegionAvail().x;
            float cellWidthPixels = size.x * s_RelativeCellSize;
            int columnCount = static_cast<int>(availableWidth / cellWidthPixels);
            if (columnCount < 1)
                columnCount = 1;
            if (ImGui::BeginTable("AssetBrowserTable", columnCount))
            {
                float iconSizePixels = size.x * s_RelativeIconSize;
                for (const auto &entry: std::filesystem::directory_iterator(m_CurrentPath))
                {
                    ImGui::TableNextColumn();

                    bool isDirectory = entry.is_directory();
                    auto iconId = isDirectory ? m_DirectoryTextureId : m_FileTextureId;

                    auto name = entry.path().filename().string();
                    ImGui::PushID(name.c_str());

                    ImGui::ImageButton("##icon", iconId, {iconSizePixels, iconSizePixels});

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        if (isDirectory)
                            m_CurrentPath /= entry.path().filename();

                    ImGui::PopID();

                    ImGui::TextWrapped("%s", name.c_str());
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();
    }

    void AssetBrowserPanel::useFullWidth(bool use)
    {
        m_UseFullWidth = use;
    }

    void AssetBrowserPanel::setDirectoryTextureId(ImTextureID id)
    {
        m_DirectoryTextureId = id;
    }

    void AssetBrowserPanel::setFileTextureId(ImTextureID id)
    {
        m_FileTextureId = id;
    }
}
