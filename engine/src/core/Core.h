#pragma once

#define KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ClassName) \
    ClassName(); \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete; \
    ClassName(ClassName&& other) noexcept; \
    ClassName& operator=(ClassName&& other) noexcept;

#define KAILUX_CHECK_DATA_STRUCTURE_SIZE(ClassName) \
    static_assert(sizeof(ClassName) % 16 == 0);

namespace kailux
{
    template<typename T>
    using Scoped = std::unique_ptr<T>;
    template<typename T>
    using Shared = std::shared_ptr<T>;

    template<typename T, typename... Args>
    constexpr Scoped<T> create_scoped(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    constexpr Shared<T> create_shared(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
}