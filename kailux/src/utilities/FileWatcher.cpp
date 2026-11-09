#include "FileWatcher.h"

namespace kailux
{
    FileWatcher::FileWatcher() = default;

    FileWatcher::FileWatcher(FileWatcher &&other) noexcept = default;

    FileWatcher &FileWatcher::operator=(FileWatcher &&other) noexcept = default;

    bool FileWatcher::Watch(const std::filesystem::path &path)
    {
        namespace fs = std::filesystem;
        Stop();

        std::error_code error;
        auto absolute{fs::absolute(path, error)};
        if (error)
            return false;

        Stamp stamp;
        if (!read_stamp(absolute, stamp))
            return false;

        mPath      = std::move(absolute);
        mStamp     = stamp;
        mNextCheck = SteadyClock::now() + kCheckInterval;
        return true;
    }

    void FileWatcher::Stop()
    {
        mPath.clear();
        mStamp = {};
        mPending = false;
    }

    bool FileWatcher::IsWatching() const
    {
        return !mPath.empty();
    }

    const std::filesystem::path & FileWatcher::GetPath() const
    {
        return mPath;
    }

    bool FileWatcher::ConsumeChange()
    {
        if (mPath.empty())
            return false;

        const auto now{SteadyClock::now()};
        if (now < mNextCheck)
            return false;
        mNextCheck = now + kCheckInterval;

        Stamp stamp;
        if (!read_stamp(mPath, stamp))
            return false;

        if (stamp != mStamp)
        {
            mStamp   = stamp;
            mPending = true;
            return false;
        }

        return std::exchange(mPending, false);
    }

    FileWatcher::~FileWatcher() = default;

    bool FileWatcher::read_stamp(const std::filesystem::path &path, Stamp &stamp)
    {
        namespace fs = std::filesystem;

        std::error_code error;
        stamp.writeTime = fs::last_write_time(path, error);
        if (error)
            return false;

        stamp.size = fs::file_size(path, error);
        return !error;
    }
}
