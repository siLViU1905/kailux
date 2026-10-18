#pragma once
#include "Core.h"
#include "scripting/ScriptRegistry.h"

namespace kailux
{
    struct NativeScriptComponent
    {
        template<std::derived_from<Script> Type>
        void Bind()
        {
            Instantiate = []() -> Scoped<Script>
            {
                return create_scoped<Type>();
            };
        }

        bool Bind(std::string_view scriptName)
        {
            const auto factory{script_registry::find(scriptName)};
            if (!factory)
                return false;

            name = scriptName;
            Instantiate = factory;
            return true;
        }

        std::string    name;
        Scoped<Script> instance;
        ScriptFactory  Instantiate{};
    };
}
