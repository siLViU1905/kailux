#pragma once
#include <optional>

namespace kailux
{
    class FileDialog
    {
    public:
        static constexpr std::string_view s_DefaultTitle = "Choose a file";
        using DialogResult = std::optional<std::vector<std::string>>;
        static DialogResult open(std::string_view title = s_DefaultTitle);
    };
}
