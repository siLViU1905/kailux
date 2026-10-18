#include "ScriptRegistry.h"

namespace kailux
{
    namespace script_registry::details
    {
        std::map<std::string, ScriptFactory, std::less<>>& factories()
        {
            static std::map<std::string, ScriptFactory, std::less<>> instance;
            return instance;
        }
    }

    bool script_registry::add(std::string_view name, ScriptFactory factory)
    {
        return details::factories().try_emplace(std::string{name}, factory).second;
    }

    ScriptFactory script_registry::find(std::string_view name)
    {
        const auto& map{details::factories()};
        const auto it{map.find(name)};
        return it == map.end() ? nullptr : it->second;
    }

    std::vector<std::string_view> script_registry::names()
    {
        const auto& map{details::factories()};
        std::vector<std::string_view> result;
        result.reserve(map.size());
        for (const auto& name : map | std::views::keys)
            result.emplace_back(name);
        return result;
    }
}
