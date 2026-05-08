#include "Console.h"

namespace kailux
{
    void Console::render()
    {
        ImGui::BeginChild("ConsoleLogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 8));

        int i = 0;
        for (const auto &[message, severity] : m_Logs)
        {
            ImGui::PushID(i++);

            ImVec2 p0 = ImGui::GetCursorScreenPos();
            float lineWeight = 3.f;
            float rowHeight = ImGui::GetTextLineHeightWithSpacing() + 4.f;
            ImVec2 p1 = ImVec2(p0.x + ImGui::GetContentRegionAvail().x, p0.y + rowHeight);


            ImGui::SetCursorScreenPos(p0);
            bool selected = false;
            ImGui::Selectable("##row", &selected, ImGuiSelectableFlags_AllowOverlap, ImVec2(0, rowHeight));

            if (ImGui::IsItemHovered())
                ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, ImGui::GetColorU32(ImGuiCol_HeaderHovered, 0.3f));


            if (ImGui::BeginPopupContextItem("LogItemCtx"))
            {
                if (ImGui::MenuItem("Copy"))
                    ImGui::SetClipboardText(get_non_formated_message(message).data());
                ImGui::EndPopup();
            }

            ImGui::GetWindowDrawList()->AddLine(ImVec2(p0.x, p1.y), p1, ImGui::GetColorU32(ImGuiCol_Separator, 0.5f));

            auto color = s_SeverityColors[static_cast<uint8_t>(severity)];
            ImGui::GetWindowDrawList()->AddRectFilled(p0, ImVec2(p0.x + lineWeight, p1.y),
                                                      ImGui::ColorConvertFloat4ToU32(color));

            ImGui::SetCursorScreenPos(ImVec2(p0.x + lineWeight + 8.f, p0.y + 2.f));
            ImGui::TextUnformatted(message.c_str());

            ImGui::SetCursorScreenPos(ImVec2(p0.x, p1.y));

            ImGui::PopID();
        }

        if (ImGui::BeginPopupContextWindow("ConsoleBackgroundCtx",
                                           ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Clear"))
                m_Logs.clear();
            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.f);

        ImGui::EndChild();
    }

    std::string_view Console::get_non_formated_message(std::string_view message)
    {
        return message.substr(message.find_first_of(' ') + 1);
    }
}
