#pragma once

#include "AssetBrowser.h"
#include "Console.h"
#include "../Panel.h"

namespace kailux
{
    class ProjectPanel : public Panel
    {
    public:
        ProjectPanel();
        ProjectPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor);

        void render(Scene &scene) override;

        void useFullWidth(bool use = true);

        AssetBrowser& getAssetBrowser();
        Console&      getConsole();

    private:
        bool         m_UseFullWidth;

        AssetBrowser m_AssetBrowser;
        Console      m_Console;
    };
}
