#include "SimulationPanel.h"

namespace kailux
{
    SimulationPanel::SimulationPanel(std::string_view name) : Panel(name, {})
    {
    }

    void SimulationPanel::render(Scene &scene)
    {
        ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{});

        constexpr auto flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar;
        mVisible = ImGui::Begin(mName.data(), &mOpen, flags);
        mFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        if (mVisible)
        {
            const auto avail = ImGui::GetContentRegionAvail();

            const auto size = (avail.x / avail.y > mAspectRatio)
                ? ImVec2{avail.y * mAspectRatio, avail.y}
                : ImVec2{avail.x, avail.x / mAspectRatio};

            ImGui::SetCursorPos(ImVec2(
                (avail.x - size.x) * 0.5f,
                (avail.y - size.y) * 0.5f
            ));
            ImGui::Image(mTextureId, size);
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void SimulationPanel::setTextureId(ImTextureID id)
    {
        mTextureId = id;
    }

    void SimulationPanel::setAspectRatio(float ratio)
    {
        mAspectRatio = ratio;
    }

    bool SimulationPanel::isVisible() const
    {
        return mVisible;
    }
}
