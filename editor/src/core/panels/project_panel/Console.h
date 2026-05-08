#pragma once

#include <magic_enum/magic_enum.hpp>

namespace kailux
{
    enum class LogSeverity : uint8_t
    {
        Info,
        Warning,
        Error
    };

    class Console
    {
    public:
        Console();

        template<LogSeverity severity>
        void log(std::string_view message)
        {
            std::string formatedMessage;
            if constexpr (severity == LogSeverity::Info)
                formatedMessage = std::format("[{}]: {}", magic_enum::enum_name(severity), message);
            else if constexpr (severity == LogSeverity::Warning)
                formatedMessage = std::format("[{}]: {}", magic_enum::enum_name(severity), message);
            else if constexpr (severity == LogSeverity::Error)
                formatedMessage = std::format("[{}]: {}", magic_enum::enum_name(severity), message);

            m_Logs.emplace_back(std::move(formatedMessage), severity);
        }

        void render();

    private:
        static constexpr std::array s_SeverityColors = {
            ImVec4(0, 1, 0, 1), // info
            ImVec4(1, 1, 0, 1), // warning
            ImVec4(1, 0, 0, 1), // error
        };

        struct Log
        {
            std::string message;
            LogSeverity severity;
        };

        std::vector<Log> m_Logs;
    };
}
