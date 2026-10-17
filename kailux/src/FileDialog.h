#pragma once
#include <optional>
#include <portable-file-dialogs.h>

#include "utilities/Queue.h"

namespace kailux
{
    enum class DialogMode
    {
        SingleFile,
        MultipleFiles,
        Folder,
        SaveFile
    };

    template<DialogMode Mode>
    class FileDialog
    {
    public:
        static constexpr std::string_view kDefaultTitle = "Choose...";
        static constexpr std::array<std::string_view, 2> kDefaultFilters =
        {
            "All Files",
            "*"
        };

        using Filters = std::vector<std::string>;

        void Open(
            std::string_view title = kDefaultTitle,
            const Filters &filters = std::ranges::to<Filters>(kDefaultFilters),
            std::string_view defaultPath = ""
        )
        {
            if constexpr (Mode == DialogMode::SingleFile)
                mDiagHandle.emplace(title.data(), "", filters);
            else if constexpr (Mode == DialogMode::MultipleFiles)
                mDiagHandle.emplace(title.data(), "", filters, pfd::opt::multiselect);
            else if constexpr (Mode == DialogMode::Folder)
                mDiagHandle.emplace(title.data(), "");
            else
                mDiagHandle.emplace(title.data(), defaultPath.data());
        }

        bool Poll()
        {
            if (mDiagHandle && mDiagHandle->ready())
            {
                if constexpr (Mode == DialogMode::Folder || Mode == DialogMode::SaveFile)
                {
                    auto result = mDiagHandle->result();
                    if (!result.empty())
                        mPathsQueue.Push(result);
                } else
                    for (const auto &path: mDiagHandle->result())
                        mPathsQueue.Push(clean_path(path));

                mDiagHandle.reset();
                return true;
            }
            return false;
        }

        using PopPathResult = std::optional<std::string>;
        PopPathResult TryPopPath()
        {
            return mPathsQueue.TryPop();
        }

    private:
        static std::string clean_path(std::string_view path)
        {
            std::string clean = path.data();
            if (path.front() == '\'' || path.front() == '"')
                clean.erase(0, 1);
            if (path.back() == '\'' || path.back() == '"')
                clean.pop_back();
            return clean;
        }

        using HandleType = std::conditional_t<
            Mode == DialogMode::Folder,
            pfd::select_folder,
            std::conditional_t<
                Mode == DialogMode::SaveFile, pfd::save_file,
                pfd::open_file>
        >;

        std::optional<HandleType> mDiagHandle;
        Queue<std::string>        mPathsQueue;
    };
}
