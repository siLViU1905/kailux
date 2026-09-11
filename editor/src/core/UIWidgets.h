#pragma once

namespace kailux::widgets
{
    namespace colors
    {
        inline constexpr auto kRowIdle      {IM_COL32(255, 255, 255, 6)};
        inline constexpr auto kRowBorder    {IM_COL32(255, 255, 255, 12)};
        inline constexpr auto kRowHover     {IM_COL32(255, 255, 255, 14)};
        inline constexpr auto kRowSelected  {IM_COL32(232, 162, 74, 38)};
        inline constexpr auto kRowSelBorder {IM_COL32(232, 162, 74, 110)};
        inline constexpr auto kAccent       {IM_COL32(232, 162, 74, 255)};
 
        inline constexpr auto kAxisX {IM_COL32(214, 88, 88, 255)};
        inline constexpr auto kAxisY {IM_COL32(128, 190, 96, 255)};
        inline constexpr auto kAxisZ {IM_COL32(88, 144, 214, 255)};
    }

    bool section(std::string_view label, bool defaultOpen = true);

    bool begin_properties(std::string_view id, float labelWidth = 104.f);
    void end_properties();
    void property(std::string_view label);

    bool vec3_control(
        std::string_view id,
        glm::vec3 &value,
        float speed = 0.05f,
        float resetValue = 0.f,
        bool *deactivatedAfterEdit = nullptr,
        std::string_view format = "%.2f",
        float width = 0.f);

    bool segmented(std::string_view id, int &current, std::span<const std::string_view> items);

    template<typename T, size_t N>
    bool segmented(
        std::string_view id,
        T &current,
        const std::array<std::string_view, N> &labels,
        const std::array<T, N> &values)
    {
        int index = 0;
        for (size_t i{}; i < N; ++i)
            if (values[i] == current)
                index = static_cast<int>(i);

        if (!segmented(id, index, labels))
            return false;

        current = values[index];
        return true;
    }

}