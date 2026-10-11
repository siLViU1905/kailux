#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <type_traits>
#include <utility>

namespace kailux
{
    class DynamicLibrary
    {
    public:
       DynamicLibrary() = default;

       DynamicLibrary(DynamicLibrary &&other) noexcept
           : mHandle(std::exchange(other.mHandle, nullptr))
       {
       }
       DynamicLibrary &operator=(DynamicLibrary &&other) noexcept
       {
           if (this != &other)
           {
               Close();
               mHandle = std::exchange(other.mHandle, nullptr);
           }
           return *this;
       }
        
        DynamicLibrary(const DynamicLibrary&)            = delete;
        DynamicLibrary& operator=(const DynamicLibrary&) = delete;

        using OpenResult = std::expected<DynamicLibrary, std::string>;
        static OpenResult open(const std::filesystem::path& path);

        using SymbolResult = std::expected<void*, std::string>;
        SymbolResult GetSymbolAddress(std::string_view name) const;

        template<typename T>
        using GetResult = std::expected<T*, std::string>;

        template<typename T>
        GetResult<T> Get(std::string_view name) const
        {
            return GetSymbolAddress(name).transform([](void* ptr)
            {
                if constexpr (std::is_function_v<T>)
                    return reinterpret_cast<T*>(ptr);
                else
                    return static_cast<T*>(ptr);
            });
        }

        void Close();

        ~DynamicLibrary()
        {
            Close();
        }

    private:
       explicit DynamicLibrary(void *handle) : mHandle(handle)
       {
       }

        void* mHandle{};
    };
}
