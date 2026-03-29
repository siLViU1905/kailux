#include "Event.h"

#include <format>
#include <magic_enum/magic_enum.hpp>

namespace kailux
{
    std::string KeyPressed::toString() const
    {
        return std::format("KeyPressed(key:{}, scancode:{}, mods:{})", magic_enum::enum_name(key), scancode, magic_enum::enum_name(mods));
    }

    std::string KeyReleased::toString() const
    {
        return std::format("KeyReleased(key:{}, scancode:{}, mods:{})", magic_enum::enum_name(key), scancode, magic_enum::enum_name(mods));
    }

    std::string KeyRepeated::toString() const
    {
        return std::format("KeyRepeated(key:{}, scancode:{}, mods:{})", magic_enum::enum_name(key), scancode, magic_enum::enum_name(mods));
    }

    std::string ButtonPressed::toString() const
    {
        return std::format("ButtonPressed(button:{}, mods:{})", magic_enum::enum_name(button), magic_enum::enum_name(mods));
    }

    std::string ButtonReleased::toString() const
    {
        return std::format("ButtonReleased(button:{}, mods:{})", magic_enum::enum_name(button), magic_enum::enum_name(mods));
    }
}
