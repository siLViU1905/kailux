#pragma once

#include <print>

#define KAILUX_LOG_CL_CLR_RESET       "\033[0m"
#define KAILUX_LOG_CL_CLR_GREEN       "\033[1;32m"
#define KAILUX_LOG_CL_CLR_YELLOW      "\033[1;33m"
#define KAILUX_LOG_CL_CLR_CYAN        "\033[1;36m"
#define KAILUX_LOG_CL_CLR_MAGENTA     "\033[1;35m"
#define KAILUX_LOG_CL_CLR_RED         "\033[1;31m"
#define KAILUX_LOG_CL_CLR_BLUE        "\033[1;34m"

#define KAILUX_LOG_PARENT(prefix, color) \
    std::println("{}{}{}", \
    color, prefix, KAILUX_LOG_CL_CLR_RESET);

#define KAILUX_LOG_CHILD(text, color) \
    std::println("  {}-{}{}", \
    color, text, KAILUX_LOG_CL_CLR_RESET);

#define KAILUX_LOG_PARENT_CLR_GREEN(prefix) KAILUX_LOG_PARENT(prefix, KAILUX_LOG_CL_CLR_GREEN)
#define KAILUX_LOG_PARENT_CLR_YELLOW(prefix) KAILUX_LOG_PARENT(prefix, KAILUX_LOG_CL_CLR_YELLOW)
#define KAILUX_LOG_PARENT_CLR_CYAN(prefix) KAILUX_LOG_PARENT(prefix, KAILUX_LOG_CL_CLR_CYAN)
#define KAILUX_LOG_PARENT_CLR_MAGENTA(prefix) KAILUX_LOG_PARENT(prefix, KAILUX_LOG_CL_CLR_MAGENTA)
#define KAILUX_LOG_PARENT_CLR_RED(prefix) KAILUX_LOG_PARENT(prefix, KAILUX_LOG_CL_CLR_RED)
#define KAILUX_LOG_PARENT_CLR_BLUE(prefix) KAILUX_LOG_PARENT(prefix, KAILUX_LOG_CL_CLR_BLUE)

#define KAILUX_LOG_CHILD_CLR_GREEN(text) KAILUX_LOG_CHILD(text, KAILUX_LOG_CL_CLR_GREEN)
#define KAILUX_LOG_CHILD_CLR_YELLOW(text) KAILUX_LOG_CHILD(text, KAILUX_LOG_CL_CLR_YELLOW)
#define KAILUX_LOG_CHILD_CLR_CYAN(text) KAILUX_LOG_CHILD(text, KAILUX_LOG_CL_CLR_CYAN)
#define KAILUX_LOG_CHILD_CLR_MAGENTA(text) KAILUX_LOG_CHILD(text, KAILUX_LOG_CL_CLR_MAGENTA)
#define KAILUX_LOG_CHILD_CLR_RED(text) KAILUX_LOG_CHILD(text, KAILUX_LOG_CL_CLR_RED)
#define KAILUX_LOG_CHILD_CLR_BLUE(text) KAILUX_LOG_CHILD(text, KAILUX_LOG_CL_CLR_BLUE)

#ifndef NDEBUG
#define KAILUX_LOG_INFO(prefix, text) \
    std::println("{}{} -> {}{}", \
    KAILUX_LOG_CL_CLR_BLUE, prefix, text, KAILUX_LOG_CL_CLR_RESET);

#define KAILUX_LOG_WARNING(prefix, text) \
    std::println("{}{} -> {}{}", \
    KAILUX_LOG_CL_CLR_YELLOW, prefix, text, KAILUX_LOG_CL_CLR_RESET);
#else
#define KAILUX_LOG_INFO(prefix, text)
#define KAILUX_LOG_WARNING(prefix, text)
#endif
