#include "ViewportPanel.h"
#include <ImGuizmo.h>

namespace kailux
{
    ViewportPanel::ViewportPanel() : mSceneTextureId(0), mSimulationState(SimulationState::Paused)
    {
    }

    void ViewportPanel::render(Scene &scene)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin("Viewport"))
        {
            auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
            auto viewportOffset = ImGui::GetWindowPos();

            ImVec2 minBound = {viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y};
            auto viewportSize = ImGui::GetContentRegionAvail();

            ImGui::Image(mSceneTextureId, viewportSize);

            if (ImGui::IsItemHovered())
            {
                mMousePos = compute_relative_mouse_pos(minBound, viewportSize);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    mOnClick();
            }

            renderSimulationIndicator(minBound, viewportSize);

            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(minBound.x, minBound.y, viewportSize.x, viewportSize.y);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ViewportPanel::setSceneTextureId(ImTextureID id)
    {
        mSceneTextureId = id;
    }

    ViewportPanel::MousePosition ViewportPanel::getScaledMousePos() const
    {
        return mMousePos;
    }

    void ViewportPanel::setOnClick(OnClick &&callback)
    {
        mOnClick = std::move(callback);
    }

    SimulationState ViewportPanel::getSimulationState() const
    {
        return mSimulationState;
    }

    void ViewportPanel::setOnSimulationStart(OnSimulationStart &&callback)
    {
        mOnSimulationStart = std::move(callback);
    }

    void ViewportPanel::setOnSimulationPause(OnSimulationPause &&callback)
    {
        mOnSimulationPause = std::move(callback);
    }

    ViewportPanel::MousePosition ViewportPanel::compute_relative_mouse_pos(ImVec2 minBound, ImVec2 viewportSize)
    {
        auto globalPos = ImGui::GetMousePos();

        float relX = globalPos.x - minBound.x;
        float relY = globalPos.y - minBound.y;

        float textureWidth = ImGui::GetIO().DisplaySize.x;
        float textureHeight = ImGui::GetIO().DisplaySize.y;

        auto scaledX = static_cast<uint32_t>((relX / viewportSize.x) * textureWidth);
        auto scaledY = static_cast<uint32_t>((relY / viewportSize.y) * textureHeight);

        return {scaledX, scaledY};
    }

    void ViewportPanel::renderSimulationIndicator(ImVec2 minBound, ImVec2 viewportSize)
    {
        constexpr float padding = 8.f;
        constexpr ImVec2 size{26.f, 26.f};
        ImVec2 pos = {minBound.x + viewportSize.x * 0.5f - size.x * 0.5f, minBound.y + padding};
        ImDrawList *dl = ImGui::GetWindowDrawList();

        dl->AddRectFilled(
            pos,
            ImVec2(pos.x + size.x, pos.y + size.y),
            IM_COL32(20, 20, 20, 120),
            4.f
        );

        if (mSimulationState == SimulationState::Paused)
        {
            float cx = pos.x + size.x * 0.5f + 1.f;
            float cy = pos.y + size.y * 0.5f;
            constexpr float r = 7.f;
            dl->AddTriangleFilled(
                ImVec2(cx - r * 0.6f, cy - r),
                ImVec2(cx - r * 0.6f, cy + r),
                ImVec2(cx + r, cy),
                IM_COL32(60, 220, 80, 255)
            );
        } else
        {
            float cx = pos.x + size.x * 0.5f;
            float cy = pos.y + size.y * 0.5f;
            constexpr float bw = 3.5f;
            constexpr float bh = 9.f;
            constexpr float gap = 3.f;

            dl->AddRectFilled(
                ImVec2(cx - gap * 0.5f - bw, cy - bh),
                ImVec2(cx - gap * 0.5f, cy + bh),
                IM_COL32(255, 255, 255, 220)
            );
            dl->AddRectFilled(
                ImVec2(cx + gap * 0.5f, cy - bh),
                ImVec2(cx + gap * 0.5f + bw, cy + bh),
                IM_COL32(255, 255, 255, 220)
            );
        }

        ImGui::SetCursorScreenPos(pos);
        if (ImGui::InvisibleButton("##sim_indicator", size))
            switch (mSimulationState)
            {
            case SimulationState::Paused:
                    mSimulationState = SimulationState::Running;
                    mOnSimulationStart();
                    break;
            case SimulationState::Running:
                    mSimulationState = SimulationState::Paused;
                    mOnSimulationPause();
                    break;
            }

        if (ImGui::IsItemHovered())
        {
            dl->AddRectFilled(
                pos,
                ImVec2(pos.x + size.x, pos.y + size.y),
                IM_COL32(255, 255, 255, 25),
                4.f
            );
            dl->AddRect(
                pos,
                ImVec2(pos.x + size.x, pos.y + size.y),
                IM_COL32(255, 255, 255, 120),
                4.f,
                0,
                1.f
            );
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
    }
}
