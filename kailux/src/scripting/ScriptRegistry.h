#pragma once
#include <concepts>
#include <string_view>
#include <vector>

#include "Script.h"

namespace kailux
{
    using ScriptFactory = Scoped<Script>(*)();

    namespace script_registry
    {
        bool                          add(std::string_view name, ScriptFactory factory);
        ScriptFactory                 find(std::string_view name);
        std::vector<std::string_view> names();

        template<std::derived_from<Script> Type>
        bool add(std::string_view name)
        {
            return add(name, []() -> Scoped<Script>
            {
                return create_scoped<Type>();
            });
        }
    }
}

#define KAILUX_REGISTER_SCRIPT(Type) \
    [[maybe_unused]] static const bool kailux_script_registered_##Type{::kailux::script_registry::add<Type>(#Type)}