#include "FileDialog.h"
#include <portable-file-dialogs.h>

namespace kailux
{
    FileDialog::DialogResult FileDialog::open(std::string_view title)
    {
        auto result = pfd::open_file(title.data());
        if (!result.result().empty())
            return {result.result()};
        return std::nullopt;
    }
}
