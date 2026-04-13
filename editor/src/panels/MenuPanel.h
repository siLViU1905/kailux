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

        using OnLoadMesh = std::move_only_function<void(std::string_view)>;
        void setOnLoadMesh(OnLoadMesh&& callback);

        void render(Scene &scene) override;

    private:
        void renderProfilerWindow();
        void sendLoadMeshPaths();

        bool       m_ShowProfiler;
        OnLoadMesh m_OnLoadMesh;
        FileDialog m_LoadMeshDialog;
    };
}
