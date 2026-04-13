#pragma once
#include <optional>
#include "utilities/Queue.h"

namespace kailux
{
    class FileDialog
    {
    public:
        static constexpr std::string_view s_DefaultTitle = "Choose a file";

        void open(std::string_view title = s_DefaultTitle);

        using PopPathResult = std::optional<std::string>;
        PopPathResult tryPopPath();

    private:
        Queue<std::string> m_PathsQueue;
    };
}
