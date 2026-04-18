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

    private:
        static constexpr float s_IconSize = 64.f;
        static constexpr float s_IconPadding = 16.f;
        static constexpr float s_CellSize = s_IconSize + s_IconPadding;

        using Path = std::filesystem::path;
        static constexpr std::string_view s_DefaultPath = "assets/";

        Path     m_CurrentPath;
    };
}
