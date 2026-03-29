#pragma once

#define KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ClassName) \
    ClassName(); \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete; \
    ClassName(ClassName&& other) noexcept; \
    ClassName& operator=(ClassName&& other) noexcept;

#define KAILUX_CHECK_DATA_STRUCTURE_SIZE(ClassName) \
    static_assert(sizeof(ClassName) % 16 == 0);