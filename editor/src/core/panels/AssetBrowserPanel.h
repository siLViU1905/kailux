#pragma once
#include <filesystem>

#include "Panel.h"

namespace kailux
{
    class AssetBrowserPanel : public Panel
    {
    public:
        AssetBrowserPanel();
        AssetBrowserPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor);

        void render(Scene &scene) override;

        void useFullWidth(bool use = true);

        void setDirectoryTextureId(ImTextureID id);
        void setFileTextureId(ImTextureID id);

        static constexpr std::string_view s_DragDropPayloadType = "CONTENT_BROWSER_ITEM";

    private:
        static constexpr float s_RelativeIconSize = 0.05f;
        static constexpr float s_RelativeIconPadding = 0.015f;
        static constexpr float s_RelativeCellSize = s_RelativeIconSize + s_RelativeIconPadding;

        static constexpr ImGuiDragDropFlags s_DragDropSourceFlags = ImGuiDragDropFlags_SourceNoDisableHover |
                                                                    ImGuiDragDropFlags_SourceNoHoldToOpenOthers |
                                                                    ImGuiDragDropFlags_SourceNoPreviewTooltip;

        using Path = std::filesystem::path;
        static constexpr std::string_view s_DefaultPath = "assets";

        Path     m_CurrentPath;
        bool     m_UseFullWidth;

        ImTextureID m_DirectoryTextureId;
        ImTextureID m_FileTextureId;
    };
}
