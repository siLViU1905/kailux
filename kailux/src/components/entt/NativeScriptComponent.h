#pragma once
#include "Core.h"
#include "scripting/ScriptRegistry.h"
#include "scripting/ScriptParams.h"

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
            name = scriptName;

            const auto factory{script_registry::find(scriptName)};
            if (!factory)
                return false;

            Instantiate = factory;
            return true;
        }

        static constexpr uint32_t kNoSourceId{~0u};

        std::string    name;
        ScriptParams   params;
        uint32_t       sourceId{kNoSourceId};
        Scoped<Script> instance;
        ScriptFactory  Instantiate{};
    };
}
