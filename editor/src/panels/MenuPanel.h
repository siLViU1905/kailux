#pragma once
#include "Panel.h"
#include "core/FileDialog.h"

namespace kailux
{
    class MenuPanel : public Panel
    {
    public:
        MenuPanel();
        MenuPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor);

        using OnLoadMesh = std::move_only_function<void()>;
        void setOnLoadMesh(OnLoadMesh&& callback);

        void render(Scene &scene) override;

    private:
        void renderProfilerWindow();

        bool       m_ShowProfiler;
        OnLoadMesh m_OnLoadMesh;
        FileDialog m_LoadMeshDialog;
    };
}
