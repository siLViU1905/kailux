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
        Folder
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

        void open(
            std::string_view title = kDefaultTitle,
            const Filters &filters = std::ranges::to<Filters>(kDefaultFilters)
        )
        {
            if constexpr (Mode == DialogMode::SingleFile)
                m_DiagHandle.emplace(title.data(), "", filters);
            else if constexpr (Mode == DialogMode::MultipleFiles)
                m_DiagHandle.emplace(title.data(), "", filters, pfd::opt::multiselect);
            else if constexpr (Mode == DialogMode::Folder)
                m_DiagHandle.emplace(title.data(), "");
        }

        bool poll()
        {
            if (m_DiagHandle && m_DiagHandle->ready())
            {
                if constexpr (Mode == DialogMode::Folder)
                {
                    auto result = m_DiagHandle->result();
                    if (!result.empty())
                        m_PathsQueue.push(result);
                } else
                    for (const auto &path: m_DiagHandle->result())
                        m_PathsQueue.push(clean_path(path));

                m_DiagHandle.reset();
                return true;
            }
            return false;
        }

        using PopPathResult = std::optional<std::string>;
        PopPathResult tryPopPath()
        {
            return m_PathsQueue.tryPop();
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
            pfd::open_file
        >;

        std::optional<HandleType> m_DiagHandle;
        Queue<std::string>        m_PathsQueue;
    };
}
