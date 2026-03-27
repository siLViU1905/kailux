#include "Event.h"

#include <format>

namespace kailux
{
    std::string KeyPressed::toString() const
    {
        return std::format("KeyPressed(key:{}, scancode:{}, mods:{})", static_cast<int>(key), static_cast<int>(scancode), static_cast<int>(mods));
    }

    std::string KeyReleased::toString() const
    {
        return std::format("KeyReleased(key:{}, scancode:{}, mods:{})", static_cast<int>(key), static_cast<int>(scancode), static_cast<int>(mods));
    }

    std::string KeyRepeated::toString() const
    {
        return std::format("KeyRepeated(key:{}, scancode:{}, mods:{})", static_cast<int>(key), static_cast<int>(scancode), static_cast<int>(mods));
    }

    std::string ButtonPressed::toString() const
    {
        return std::format("ButtonPressed(button:{}, mods:{})", static_cast<int>(button), static_cast<int>(mods));
    }

    std::string ButtonReleased::toString() const
    {
        return std::format("ButtonReleased(button:{}, mods:{})", static_cast<int>(button), static_cast<int>(mods));
    }
}
