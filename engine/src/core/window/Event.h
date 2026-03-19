#pragma once
#include <string>
#include <variant>

namespace kailux
{
    struct KeyPressed;
    struct KeyReleased;
    struct KeyRepeated;

    using Event = std::variant<KeyPressed, KeyReleased, KeyRepeated>;

    struct KeyPressed
    {
        int key;
        int scancode;
        int mods;
        std::string toString() const;
    };

    struct KeyReleased
    {
        int key;
        int scancode;
        int mods;
        std::string toString() const;
    };

    struct KeyRepeated
    {
        int key;
        int scancode;
        int mods;
        std::string toString() const;
    };
}