#include "ViewportPanel.h"
#include <ImGuizmo.h>

namespace kailux
{
    ViewportPanel::ViewportPanel() : m_SceneTextureId(0)
    {
    }

    void ViewportPanel::render(Scene &scene)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin("Viewport"))
        {
            auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
            auto viewportOffset = ImGui::GetWindowPos();

            ImVec2 minBound = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
            auto viewportSize = ImGui::GetContentRegionAvail();

            ImGui::Image(m_SceneTextureId, viewportSize);

            if (ImGui::IsItemHovered())
            {
                m_MousePos = compute_relative_mouse_pos(minBound, viewportSize);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    m_OnClick();
            }

            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(minBound.x, minBound.y, viewportSize.x, viewportSize.y);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ViewportPanel::setSceneTextureId(ImTextureID id)
    {
        m_SceneTextureId = id;
    }

    ViewportPanel::MousePosition ViewportPanel::getScaledMousePos() const
    {
        return m_MousePos;
    }

    void ViewportPanel::setOnClick(OnClick &&callback)
    {
        m_OnClick = std::move(callback);
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
}
