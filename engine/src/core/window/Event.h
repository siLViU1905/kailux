#pragma once
#include <string>
#include <variant>

namespace kailux
{
    template<class... Events>
    struct EventOverloads : Events...
    {
        using Events::operator()...;
    };

    struct KeyPressed;
    struct KeyReleased;
    struct KeyRepeated;
    struct ButtonPressed;
    struct ButtonReleased;

    using Event = std::variant<KeyPressed, KeyReleased, KeyRepeated, ButtonPressed, ButtonReleased>;

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

    struct ButtonPressed
    {
        int button;
        int mods;

        std::string toString() const;
    };

    struct ButtonReleased
    {
        int button;
        int mods;

        std::string toString() const;
    };
}
