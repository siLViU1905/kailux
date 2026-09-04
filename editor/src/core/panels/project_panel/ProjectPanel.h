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
        ProjectPanel(std::string_view name, ImVec4 backgroundColor);

        void Render(Scene &scene) override;

        void UseFullWidth(bool use = true);

        AssetBrowser& GetAssetBrowser();
        Console&      GetConsole();

    private:
        bool         mUseFullWidth;

        AssetBrowser mAssetBrowser;
        Console      mConsole;
    };
}
