#include "Console.h"

namespace kailux
{
    void Console::render()
    {
        ImGui::BeginChild("ConsoleLogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::TextColored(ImVec4(0, 1, 0, 1), "[INFO]: This is an info log.");
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "[WARN]: This is a warn log.");
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "[ERROR]: SThis is an error log.");

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.f);

        ImGui::EndChild();
    }
}
