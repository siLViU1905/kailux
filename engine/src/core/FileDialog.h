#pragma once
#include <optional>

namespace kailux
{
    class FileDialog
    {
    public:
        using DialogResult = std::optional<std::vector<std::string>>;
        static DialogResult open(std::string_view title);
    };
}
