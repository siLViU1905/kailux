#ifdef _WIN32

#include "DynamicLibrary.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <format>

namespace kailux
{
    namespace details
    {
        std::string format_win32_error(DWORD code)
        {
            char* buf{};
            const auto len{FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                code,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPSTR>(&buf),
                0,
                nullptr
                )};

            std::string msg;
            if (len && buf)
            {
                msg.assign(buf, len);
                while (!msg.empty() && (msg.back() == '\r' || msg.back() == '\n' || msg.back() == ' '))
                    msg.pop_back();
            } else
                msg = std::format("Win32 error code: '{}'", code);

            if (buf)
                LocalFree(buf);
            return msg;
        }
    }

    DynamicLibrary::OpenResult DynamicLibrary::open(const std::filesystem::path &path)
    {
        DWORD oldMode{};
        SetThreadErrorMode(SEM_FAILCRITICALERRORS, &oldMode);

        const auto handle{LoadLibraryW(path.c_str())};
        const auto err{handle ? 0 : GetLastError()};

        SetThreadErrorMode(oldMode, nullptr);

        if (!handle)
            return std::unexpected{std::format(
                "dynamic library: '{}' failed to load, reason: {}",
                path.string(), details::format_win32_error(err)
                )};

        return DynamicLibrary{static_cast<void*>(handle)};
    }

    DynamicLibrary::SymbolResult DynamicLibrary::GetSymbolAddress(std::string_view name) const
    {
        if (!pHandle)
            return std::unexpected{"dynamic library: library not loaded"};

        const auto proc{GetProcAddress(static_cast<HMODULE>(pHandle), std::string{name}.c_str())};
        if (!proc)
            return std::unexpected{
                std::format(
                    "dynamic library: symbol '{}' not found",
                    name)
            };
        return reinterpret_cast<void*>(proc);
    }

    void DynamicLibrary::Close()
    {
        if (pHandle)
        {
            FreeLibrary(static_cast<HMODULE>(pHandle));
            pHandle = nullptr;
        }
    }
}

#endif
