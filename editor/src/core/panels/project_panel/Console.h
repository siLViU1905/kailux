#pragma once

#include <magic_enum/magic_enum.hpp>

namespace kailux
{
    enum class LogSeverity
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
            if constexpr (severity == LogSeverity::Info)
                m_Logs.emplace_back(
                    std::format("[{}]: {}", magic_enum::enum_name(severity), message),
                    s_InfoSeverityColor
                );
            else if constexpr (severity == LogSeverity::Warning)
                m_Logs.emplace_back(
                    std::format("[{}]: {}", magic_enum::enum_name(severity), message),
                    s_WarningSeverityColor
                );
            else if constexpr (severity == LogSeverity::Error)
                m_Logs.emplace_back(
                    std::format("[{}]: {}", magic_enum::enum_name(severity), message),
                    s_ErrorSeverityColor
                );
        }

        void render();

    private:
        static constexpr ImVec4 s_InfoSeverityColor = {0, 1, 0, 1};
        static constexpr ImVec4 s_WarningSeverityColor = {1, 1, 0, 1};
        static constexpr ImVec4 s_ErrorSeverityColor = {1, 0, 0, 1};

        struct Log
        {
            std::string message;
            ImVec4 color;
        };

        std::vector<Log> m_Logs;
    };
}
