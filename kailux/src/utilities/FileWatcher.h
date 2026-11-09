#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <utility>

#include "Core.h"

namespace kailux
{
    class FileWatcher
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(FileWatcher)

        static constexpr std::chrono::milliseconds kCheckInterval{250};

        bool Watch(const std::filesystem::path& path);
        void Stop();

        bool                         IsWatching() const;
        const std::filesystem::path& GetPath() const;

        bool ConsumeChange();

        ~FileWatcher();

    private:
        struct Stamp
        {
            std::filesystem::file_time_type writeTime{};
            std::uintmax_t                  size{};

            bool operator==(const Stamp &) const = default;
        };

        static bool read_stamp(const std::filesystem::path& path, Stamp& stamp);

        using SteadyClock = std::chrono::steady_clock;

        std::filesystem::path   mPath;
        Stamp                   mStamp;
        bool                    mPending{};
        SteadyClock::time_point mNextCheck{};
    };
}