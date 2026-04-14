#include "FileDialog.h"
#include <portable-file-dialogs.h>

namespace kailux
{
    void FileDialog::open(std::string_view title)
    {
        m_DiagHandle = create_scoped<pfd::open_file>(title.data());
    }

    bool FileDialog::poll()
    {
        if (m_DiagHandle && m_DiagHandle->ready())
        {
            for (const auto& path : m_DiagHandle->result())
                m_PathsQueue.push(path);
            m_DiagHandle.reset();
            return true;
        }
        return false;
    }

    FileDialog::PopPathResult FileDialog::tryPopPath()
    {
        return m_PathsQueue.tryPop();
    }
}
