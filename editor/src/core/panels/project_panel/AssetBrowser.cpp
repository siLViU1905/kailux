#include "AssetBrowser.h"

namespace kailux
{
    AssetBrowser::AssetBrowser() : m_CurrentPath(s_DefaultPath),
                                   m_DirectoryTextureId(0),
                                   m_FileTextureId(0),
                                   m_ItemToRenamePath(""),
                                   m_RenameBuffer(""),
                                   m_IsRenaming(false)
    {
        if (!std::filesystem::exists(m_CurrentPath))
            std::filesystem::create_directory(m_CurrentPath);
    }

    void AssetBrowser::render()
    {
        if (m_CurrentPath != s_DefaultPath)
        {
            if (ImGui::Button("<- Back"))
                m_CurrentPath = m_CurrentPath.parent_path();
            ImGui::SameLine();
        }

        ImGui::Text("%s", m_CurrentPath.string().c_str());

        float availableWidth = ImGui::GetContentRegionAvail().x;
        float cellWidthPixels = ImGui::GetWindowWidth() * s_RelativeCellSize;
        int columnCount = static_cast<int>(availableWidth / cellWidthPixels);
        if (columnCount < 1)
            columnCount = 1;
        if (ImGui::BeginTable("AssetBrowserTable", columnCount))
        {
            float iconSizePixels = ImGui::GetWindowWidth() * s_RelativeIconSize;
            for (const auto &entry: std::filesystem::directory_iterator(m_CurrentPath))
            {
                ImGui::TableNextColumn();

                bool isDirectory = std::filesystem::is_directory(entry.path());
                auto iconId = isDirectory ? m_DirectoryTextureId : m_FileTextureId;

                auto name = entry.path().filename().string();
                ImGui::PushID(name.c_str());

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));

                ImGui::ImageButton("##icon", iconId, {iconSizePixels, iconSizePixels});

                bool iconDoubleClicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Rename"))
                    {
                        m_IsRenaming = true;
                        m_ItemToRenamePath = entry.path();
                        m_RenameBuffer.fill(0);
                        std::strncpy(m_RenameBuffer.data(), name.c_str(), m_RenameBuffer.size());
                    }
                    if (ImGui::MenuItem("Delete"))
                        std::filesystem::remove_all(entry.path());

                    ImGui::EndPopup();
                }

                ImGui::PopStyleColor(3);

                if (!isDirectory)
                    if (ImGui::BeginDragDropSource(s_DragDropSourceFlags))
                    {
                        std::string itemPath = entry.path().string();
                        ImGui::SetDragDropPayload(s_DragDropPayloadType.data(), itemPath.c_str(), itemPath.size() + 1);
                        ImGui::Text("%s", name.c_str());
                        ImGui::EndDragDropSource();
                    }

                if (m_IsRenaming && m_ItemToRenamePath == entry.path())
                {
                    ImGui::SetKeyboardFocusHere();
                    if (ImGui::InputText("##rename", m_RenameBuffer.data(), m_RenameBuffer.size(),
                                         ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        std::filesystem::path newPath = entry.path().parent_path() / std::string(
                                                            m_RenameBuffer.begin(), m_RenameBuffer.end());

                        if (!std::filesystem::exists(newPath))
                            std::filesystem::rename(entry.path(), newPath);

                        m_IsRenaming = false;
                        m_ItemToRenamePath = "";
                    }

                    if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        m_IsRenaming = false;
                } else
                    ImGui::TextWrapped("%s", name.c_str());

                if (iconDoubleClicked && isDirectory)
                    m_CurrentPath /= entry.path().filename();

                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        if (ImGui::BeginPopupContextWindow("AssetBrowserContextMenu",
                                           ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("New Folder"))
            {
                Path newFolderPath = m_CurrentPath / "New Folder";
                int counter = 1;

                while (std::filesystem::exists(newFolderPath))
                {
                    newFolderPath = m_CurrentPath / ("New Folder (" + std::to_string(counter) + ")");
                    counter++;
                }
                std::filesystem::create_directory(newFolderPath);

                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Import files"))
                m_OnImportFiles();
            if (ImGui::MenuItem("Import folder"))
                m_OnImportFolder();

            ImGui::EndPopup();
        }
    }

    void AssetBrowser::setDirectoryTextureId(ImTextureID id)
    {
        m_DirectoryTextureId = id;
    }

    void AssetBrowser::setFileTextureId(ImTextureID id)
    {
        m_FileTextureId = id;
    }

    void AssetBrowser::setOnImportFiles(OnImport &&callback)
    {
        m_OnImportFiles = std::move(callback);
    }

    void AssetBrowser::setOnImportFolder(OnImport &&callback)
    {
        m_OnImportFolder = std::move(callback);
    }

    void AssetBrowser::import(std::string_view path) const
    {
        namespace fs = std::filesystem;
        if (fs::exists(path))
        {
            auto dest = m_CurrentPath;
            if (fs::is_directory(path))
                dest /= Path(path).filename();
            fs::copy(path, dest, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        }
    }
}
