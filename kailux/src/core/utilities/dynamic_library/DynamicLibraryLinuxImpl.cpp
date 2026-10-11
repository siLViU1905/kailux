#ifdef __unix__

#include "DynamicLibrary.h"

#include <dlfcn.h>
#include <format>

namespace kailux
{
    namespace details
    {
        static std::string last_dl_error(std::string_view fallback)
        {
            const char* const error{dlerror()};
            return error ? std::string{error} : std::string{fallback};
        }
    }

    DynamicLibrary::OpenResult DynamicLibrary::open(const std::filesystem::path &path)
    {
        dlerror();
        void* handle{dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL)};
        if (!handle)
            return std::unexpected{
                std::format(
                    "dynamic library: '{}' failed to load, reason: {}",
                    path.string(), details::last_dl_error("dlopen failed"))
            };
        return DynamicLibrary{handle};
    }

    DynamicLibrary::SymbolResult DynamicLibrary::GetSymbolAddress(std::string_view name) const
    {
        if (!pHandle)
            return std::unexpected{"dynamic library: library not loaded"};

        dlerror();
        void* ptr{dlsym(pHandle, std::string{name}.c_str())};
        if (!ptr)
            return std::unexpected{
                std::format(
                    "dynamic library: symbol '{}' not found",
                    name)
            };
        return ptr;
    }

    void DynamicLibrary::Close()
    {
        if (pHandle)
        {
            dlclose(pHandle);
            pHandle = nullptr;
        }
    }
}

#endif
