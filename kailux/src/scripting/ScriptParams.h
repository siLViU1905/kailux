#pragma once

#include <string>
#include <string_view>
#include <nlohmann/json.hpp>
#include "utilities/GlmJson.h"

namespace kailux
{
    class ScriptParams
    {
    public:
        ScriptParams() = default;
        explicit ScriptParams(nlohmann::json values) : mValues(values.is_object() ? std::move(values) : nlohmann::json::object())
        {
        }

        bool Has(std::string_view key) const
        {
            return mValues.contains(std::string{key});
        }

        template<typename T>
        T Get(std::string_view key, T fallback) const
        {
            const auto it{mValues.find(std::string{key})};
            if (it == mValues.end())
                return fallback;
            try
            {
                return it->template get<T>();
            }
            catch (const nlohmann::json::exception &)
            {
                return fallback;
            }
        }
        const nlohmann::json& Raw() const
        {
            return mValues;
        }

    private:
        nlohmann::json mValues{nlohmann::json::object()};
    };
}