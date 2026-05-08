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
            auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
            auto viewportOffset = ImGui::GetWindowPos();

            ImVec2 minBound = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
            ImVec2 viewportSize = { viewportMaxRegion.x - viewportMinRegion.x, viewportMaxRegion.y - viewportMinRegion.y };

            ImGui::Image(m_SceneTextureId, viewportSize);

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
}
