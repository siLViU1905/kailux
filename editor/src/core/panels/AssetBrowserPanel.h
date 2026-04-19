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

    private:
        static constexpr float s_RelativeIconSize = 0.1f;
        static constexpr float s_RelativeIconPadding = 0.1f;
        static constexpr float s_RelativeCellSize = s_RelativeIconSize + s_RelativeIconPadding;

        using Path = std::filesystem::path;
        static constexpr std::string_view s_DefaultPath = "assets";

        Path     m_CurrentPath;
        bool     m_UseFullWidth;

        ImTextureID m_DirectoryTextureId;
        ImTextureID m_FileTextureId;
    };
}
