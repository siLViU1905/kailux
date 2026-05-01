#pragma once
#include <optional>
#include <portable-file-dialogs.h>

#include "utilities/Queue.h"

namespace kailux
{
    class FileDialog
    {
    public:
        static constexpr std::string_view s_DefaultTitle = "Choose a file";
        static constexpr std::array<std::string_view, 2> s_DefaultFilters =
        {
            "All Files",
            "*"
        };

        using Filters = std::vector<std::string>;
        void open(
            std::string_view title = s_DefaultTitle,
            const Filters& filters = std::ranges::to<Filters>(s_DefaultFilters)
        );

        bool poll();

        using PopPathResult = std::optional<std::string>;

        PopPathResult tryPopPath();

    private:
        Scoped<pfd::open_file> m_DiagHandle;
        Queue<std::string>     m_PathsQueue;
    };
}
