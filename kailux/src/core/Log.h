#pragma once

#include <iostream>
#include <mutex>
#include <chrono>
#include <source_location>
#include <fstream>
#include "Core.h"

namespace kailux::log
{
    enum class Level : uint8_t
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error
    };

    namespace details
    {
        consteval Level get_compiled_log_level()
        {
            if constexpr (kailux::details::kCompiledLevel == kailux::details::CompileLevel::Debug)
                return Level::Trace;
            else if constexpr (kailux::details::kCompiledLevel == kailux::details::CompileLevel::Release)
                return Level::Info;
            return Level::Trace;
        }

        struct Style
        {
            std::string_view level;
            std::string_view color;
        };

        constexpr Style get_level_style(Level level)
        {
            switch (level)
            {
                case Level::Trace:
                    return {"trace", "\033[0;37m"};
                    break;
                case Level::Debug:
                    return {"debug", "\033[1;36m"};
                    break;
                case Level::Info:
                    return {"info", "\033[1;32m"};
                    break;
                case Level::Warning:
                    return {"warning", "\033[1;33m"};
                    break;
                case Level::Error:
                    return {"error", "\033[1;31m"};
                    break;
                default:
                    return {};
            }
        }

        template<typename... Args>
        struct FmtLoc
        {
            template<typename String>
            consteval FmtLoc(const String &s, std::source_location loc = std::source_location::current())
                : fmt{s}, loc(loc)
            {
            }

            std::format_string<Args...> fmt;
            std::source_location        loc;
        };

        template<typename... Args>
        using Fmt = FmtLoc<std::type_identity_t<Args>...>;


        inline std::ofstream gLogOutFile;
    }

    class Logger
    {
    public:
        Logger(std::ostream &out, bool color, bool timestamp) : mOut(out),
                                                                mColor(color),
                                                                mTimestamp(timestamp)
        {
        }

        template <typename... Args>
        void Trace(details::Fmt<Args...> fmt, Args&&... args) { Log<Level::Trace>(fmt, std::forward<Args>(args)...); }
        template <typename... Args>
        void Debug(details::Fmt<Args...> fmt, Args&&... args) { Log<Level::Debug>(fmt, std::forward<Args>(args)...); }
        template <typename... Args>
        void Info(details::Fmt<Args...> fmt, Args&&... args)  { Log<Level::Info>(fmt, std::forward<Args>(args)...); }
        template <typename... Args>
        void Warning(details::Fmt<Args...> fmt, Args&&... args)  { Log<Level::Warning>(fmt, std::forward<Args>(args)...); }
        template <typename... Args>
        void Error(details::Fmt<Args...> fmt, Args&&... args) { Log<Level::Error>(fmt, std::forward<Args>(args)...); }

    private:
        template<Level L, typename... Args>
        void Log(details::Fmt<Args...> fmt, Args&&... args)
        {
            if constexpr (L >= details::get_compiled_log_level())
                Write<L>(fmt.loc, std::format(fmt.fmt, std::forward<Args>(args)...));
        }

        template<Level L>
        void Write(const std::source_location& loc, std::string_view msg)
        {
            std::string line;
            line.reserve(msg.size() + 64);

            constexpr auto style{details::get_level_style(L)};
            if (mColor)
                line += style.color;
            if (mTimestamp)
            {
                auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
                line += std::format("{:%F %T} ", now);
            }
            line += std::format("[{:<5}] {}", style.level, msg);
            if constexpr (L >= Level::Warning)
                line += std::format("  ({}:{})", loc.file_name(), loc.line());
            if (mColor)
                line += "\033[0m";
            line += '\n';

            std::lock_guard lock{mMutex};
            std::ostream& out = mOut;
            out.write(line.data(), static_cast<std::streamsize>(line.size()));
            if (L >= Level::Warning)
                out.flush();
        }

        std::reference_wrapper<std::ostream> mOut;
        bool                                 mColor{};
        bool                                 mTimestamp{};
        std::mutex                           mMutex;
    };

    inline Logger console{std::clog, true, false};
    inline Logger file{details::gLogOutFile, false, true};

    inline void open_file(const std::filesystem::path& path, bool append = false)
    {
        details::gLogOutFile.open(path, append ? std::ios::app : std::ios::trunc);
    }

    inline void close_file()
    {
        details::gLogOutFile.close();
    }
}