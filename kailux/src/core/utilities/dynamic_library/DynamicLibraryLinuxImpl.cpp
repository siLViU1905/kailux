#include "DynamicLibrary.h"

#include <dlfcn.h>

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
        if (!mHandle)
            return std::unexpected{"dynamic library: library not loaded"};

        dlerror();
        void* ptr{dlsym(mHandle, name.data())};
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
        if (mHandle)
        {
            dlclose(mHandle);
            mHandle = nullptr;
        }
    }
}
