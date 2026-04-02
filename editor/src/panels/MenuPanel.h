#pragma once
#include "Panel.h"

namespace kailux
{
    class MenuPanel : public Panel
    {
    public:
        MenuPanel();
        MenuPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor);

        void render(Scene &scene) override;

    private:
        void renderProfilerWindow();

        bool m_ShowProfiler;
    };
}
