#include "AssetBrowser.h"

namespace kailux
{
    AssetBrowser::AssetBrowser() : mCurrentPath(s_DefaultPath),
                                   mDirectoryTextureId(0),
                                   mFileTextureId(0),
                                   mItemToRenamePath(""),
                                   mRenameBuffer(""),
                                   mIsRenaming(false)
    {
        if (!std::filesystem::exists(mCurrentPath))
            std::filesystem::create_directory(mCurrentPath);
    }

    void AssetBrowser::render()
    {
        if (mCurrentPath != s_DefaultPath)
        {
            if (ImGui::Button("<- Back"))
                mCurrentPath = mCurrentPath.parent_path();
            ImGui::SameLine();
        }

        ImGui::Text("%s", mCurrentPath.string().c_str());

        float availableWidth = ImGui::GetContentRegionAvail().x;
        float cellWidthPixels = ImGui::GetWindowWidth() * s_RelativeCellSize;
        int columnCount = static_cast<int>(availableWidth / cellWidthPixels);
        if (columnCount < 1)
            columnCount = 1;
        if (ImGui::BeginTable("AssetBrowserTable", columnCount))
        {
            float iconSizePixels = ImGui::GetWindowWidth() * s_RelativeIconSize;
            for (const auto &entry: std::filesystem::directory_iterator(mCurrentPath))
            {
                ImGui::TableNextColumn();

                bool isDirectory = std::filesystem::is_directory(entry.path());
                auto iconId = isDirectory ? mDirectoryTextureId : mFileTextureId;

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
                        mIsRenaming = true;
                        mItemToRenamePath = entry.path();
                        mRenameBuffer.fill(0);
                        std::strncpy(mRenameBuffer.data(), name.c_str(), mRenameBuffer.size());
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

                if (mIsRenaming && mItemToRenamePath == entry.path())
                {
                    ImGui::SetKeyboardFocusHere();
                    if (ImGui::InputText("##rename", mRenameBuffer.data(), mRenameBuffer.size(),
                                         ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        std::filesystem::path newPath = entry.path().parent_path() / std::string(
                                                            mRenameBuffer.begin(), mRenameBuffer.end());

                        if (!std::filesystem::exists(newPath))
                            std::filesystem::rename(entry.path(), newPath);

                        mIsRenaming = false;
                        mItemToRenamePath = "";
                    }

                    if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        mIsRenaming = false;
                } else
                    ImGui::TextWrapped("%s", name.c_str());

                if (iconDoubleClicked && isDirectory)
                    mCurrentPath /= entry.path().filename();

                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        if (ImGui::BeginPopupContextWindow("AssetBrowserContextMenu",
                                           ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("New Folder"))
            {
                Path newFolderPath = mCurrentPath / "New Folder";
                int counter = 1;

                while (std::filesystem::exists(newFolderPath))
                {
                    newFolderPath = mCurrentPath / ("New Folder (" + std::to_string(counter) + ")");
                    counter++;
                }
                std::filesystem::create_directory(newFolderPath);

                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Import files"))
                mOnImportFiles();
            if (ImGui::MenuItem("Import folder"))
                mOnImportFolder();

            ImGui::EndPopup();
        }
    }

    void AssetBrowser::setDirectoryTextureId(ImTextureID id)
    {
        mDirectoryTextureId = id;
    }

    void AssetBrowser::setFileTextureId(ImTextureID id)
    {
        mFileTextureId = id;
    }

    void AssetBrowser::setOnImportFiles(OnImport &&callback)
    {
        mOnImportFiles = std::move(callback);
    }

    void AssetBrowser::setOnImportFolder(OnImport &&callback)
    {
        mOnImportFolder = std::move(callback);
    }

    void AssetBrowser::import(std::string_view path) const
    {
        namespace fs = std::filesystem;
        if (fs::exists(path))
        {
            auto dest = mCurrentPath;
            if (fs::is_directory(path))
                dest /= Path(path).filename();
            fs::copy(path, dest, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        }
    }
}
