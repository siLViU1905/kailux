#pragma once

#include <imgui.h>

namespace kailux
{
    class AssetBrowser
    {
    public:
        AssetBrowser();

        void render();

        void setDirectoryTextureId(ImTextureID id);
        void setFileTextureId(ImTextureID id);

        static constexpr std::string_view s_DefaultPath = "workspace";

        static constexpr std::string_view s_DragDropPayloadType = "CONTENT_BROWSER_ITEM";

        using OnImport = std::move_only_function<void()>;
        void setOnImportFiles(OnImport&& callback);
        void setOnImportFolder(OnImport&& callback);

        void import(std::string_view path) const;

    private:
        static constexpr float s_RelativeIconSize = 0.05f;
        static constexpr float s_RelativeIconPadding = 0.015f;
        static constexpr float s_RelativeCellSize = s_RelativeIconSize + s_RelativeIconPadding;

        static constexpr ImGuiDragDropFlags s_DragDropSourceFlags = ImGuiDragDropFlags_SourceNoDisableHover |
                                                                    ImGuiDragDropFlags_SourceNoHoldToOpenOthers |
                                                                    ImGuiDragDropFlags_SourceNoPreviewTooltip;

        using Path = std::filesystem::path;

        Path     mCurrentPath;

        OnImport mOnImportFiles;
        OnImport mOnImportFolder;

        ImTextureID mDirectoryTextureId;
        ImTextureID mFileTextureId;

        Path                  mItemToRenamePath;
        std::array<char, 65>  mRenameBuffer;
        bool                  mIsRenaming;
    };
}
