#include "UIWidgets.h"

namespace kailux::widgets
{
    bool section(std::string_view label, bool defaultOpen)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.f, 8.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(1.f, 1.f, 1.f, 0.035f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.f, 1.f, 1.f, 0.055f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(1.f, 1.f, 1.f, 0.075f));

        ImGui::Spacing();
        const bool open{ImGui::CollapsingHeader(label.data(), defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None)};

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        if (open)
            ImGui::Dummy(ImVec2(0.f, 2.f));
        return open;
    }

    bool begin_properties(std::string_view id, float labelWidth)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.f, 4.f));
        if (!ImGui::BeginTable(id.data(), 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX))
        {
            ImGui::PopStyleVar();
            return false;
        }
        ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
        ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);
        return true;
    }

    void end_properties()
    {
        ImGui::EndTable();
        ImGui::PopStyleVar();
    }

    void property(std::string_view label)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", label.data());
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-std::numeric_limits<float>::min());
    }

    bool vec3_control(
        std::string_view id,
        glm::vec3 &value,
        float speed,
        float resetValue,
        bool *deactivatedAfterEdit,
        std::string_view format,
        float width)
    {
        bool changed{};
        bool deactivated{};

        ImGui::PushID(id.data());
        const float spacing = 4.f;
        const float fullWidth = width > 0.f ? width : ImGui::GetContentRegionAvail().x;
        const float labelSize = ImGui::GetFrameHeight();
        const float fieldWidth = (fullWidth - 3.f * labelSize - 2.f * spacing) / 3.f;

        constexpr const char *kAxisNames[] = {"X", "Y", "Z"};
        constexpr ImU32 kAxisColors[] = {colors::kAxisX, colors::kAxisY, colors::kAxisZ};

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);

        for (int i = 0; i < 3; ++i)
        {
            if (i > 0)
                ImGui::SameLine(0.f, spacing);

            const ImVec4 axis = ImGui::ColorConvertU32ToFloat4(kAxisColors[i]);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(axis.x, axis.y, axis.z, 0.22f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(axis.x, axis.y, axis.z, 0.40f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(axis.x, axis.y, axis.z, 0.60f));
            ImGui::PushStyleColor(ImGuiCol_Text, axis);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);

            if (ImGui::Button(kAxisNames[i], ImVec2(labelSize, labelSize)))
            {
                value[i] = resetValue;
                changed = true;
                deactivated = true;
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);

            ImGui::SameLine();
            ImGui::SetNextItemWidth(fieldWidth);
            ImGui::PushID(i);
            changed |= ImGui::DragFloat("##v", &value[i], speed, 0.f, 0.f, format.data());
            deactivated |= ImGui::IsItemDeactivatedAfterEdit();
            ImGui::PopID();
        }

        ImGui::PopStyleVar(2);
        ImGui::PopID();

        if (deactivatedAfterEdit)
            *deactivatedAfterEdit = deactivated;
        return changed;
    }

    bool segmented(std::string_view id, int &current, std::span<const std::string_view> items)
    {
        bool changed{};
        ImGui::PushID(id.data());

        constexpr float spacing{2.f};
        const float width = (ImGui::GetContentRegionAvail().x - spacing * (items.size() - 1)) / static_cast<float>(items.size());

        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);

        for (int i = 0; i < static_cast<int>(items.size()); ++i)
        {
            if (i > 0)
                ImGui::SameLine(0.f, spacing);

            const bool active{current == i};
            if (active)
            {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.91f, 0.64f, 0.29f, 0.30f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.91f, 0.64f, 0.29f, 0.38f));
                ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.98f, 0.82f, 0.60f, 1.f));
            }

            if (ImGui::Button(items[i].data(), ImVec2(width, 0.f)) && !active)
            {
                current = i;
                changed = true;
            }

            if (active)
                ImGui::PopStyleColor(3);
        }

        ImGui::PopStyleVar(2);
        ImGui::PopID();
        return changed;
    }
}
