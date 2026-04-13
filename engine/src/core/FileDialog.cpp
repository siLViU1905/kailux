#include "FileDialog.h"
#include <portable-file-dialogs.h>

namespace kailux
{
    void FileDialog::open(std::string_view title)
    {
        auto result = pfd::open_file(title.data());
        const auto& paths = result.result();
        for (const auto& path : paths)
            m_PathsQueue.push(path);
    }

    FileDialog::PopPathResult FileDialog::tryPopPath()
    {
        return m_PathsQueue.tryPop();
    }
}
