#include "Console.h"

namespace kailux
{
    Console::Console()
    {
        log<LogSeverity::Info>("This is an info");
        log<LogSeverity::Warning>("This is a warning");
        log<LogSeverity::Error>("This is an error");
    }

    void Console::render()
    {
        ImGui::BeginChild("ConsoleLogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 8));

        for (const auto&[message, color] : m_Logs)
        {
            ImGui::BeginGroup();

            ImVec2 p0 = ImGui::GetCursorScreenPos();
            float lineWeight = 3.f;
            float rowHeight = ImGui::GetTextLineHeightWithSpacing() + 4.f;
            ImVec2 p1 = ImVec2(p0.x + ImGui::GetContentRegionAvail().x, p0.y + rowHeight);
            
            ImGui::PushID(message.c_str());
            bool hovered = ImGui::IsMouseHoveringRect(p0, p1);
            if (hovered)
                ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, ImGui::GetColorU32(ImGuiCol_HeaderHovered, 0.3f));
            
            ImGui::GetWindowDrawList()->AddLine(ImVec2(p0.x, p1.y), p1, ImGui::GetColorU32(ImGuiCol_Separator, 0.5f));
            
            ImGui::GetWindowDrawList()->AddRectFilled(p0, ImVec2(p0.x + lineWeight, p1.y), ImGui::ColorConvertFloat4ToU32(color));
            
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + lineWeight + 8.f);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.f);

            ImGui::TextUnformatted(message.c_str());

            ImGui::EndGroup();
            ImGui::PopID();
        }

        ImGui::PopStyleVar();

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.f);

        ImGui::EndChild();
    }
}
