#include "SimulationPanel.h"

namespace kailux
{
    SimulationPanel::SimulationPanel(std::string_view name) : Panel(name, {})
    {
    }

    void SimulationPanel::Render(Scene &scene)
    {
        ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{});

        constexpr auto flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar;
        mVisible = ImGui::Begin(mName.data(), &mOpen, flags);
        mFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        mPlatformWindow = static_cast<GLFWwindow*>(ImGui::GetWindowViewport()->PlatformHandle);

        if (mVisible)
        {
            mExtent = ImGui::GetContentRegionAvail();
            if (mTextureId)
                ImGui::Image(mTextureId, mExtent);
            else
                ImGui::Dummy(mExtent);

            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
                mToggleMouseLook = true;
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void SimulationPanel::SetTextureId(ImTextureID id)
    {
        mTextureId = id;
    }

    bool SimulationPanel::IsVisible() const
    {
        return mVisible;
    }
}
