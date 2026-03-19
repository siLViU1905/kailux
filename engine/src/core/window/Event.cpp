#include "Event.h"

#include <format>

namespace kailux
{
    std::string KeyPressed::toString() const
    {
        return std::format("KeyPressed(key:{}, scancode:{}, mods:{})", key, scancode, mods);
    }

    std::string KeyReleased::toString() const
    {
        return std::format("KeyReleased(key:{}, scancode:{}, mods:{})", key, scancode, mods);
    }

    std::string KeyRepeated::toString() const
    {
        return std::format("KeyRepeated(key:{}, scancode:{}, mods:{})", key, scancode, mods);
    }
}
