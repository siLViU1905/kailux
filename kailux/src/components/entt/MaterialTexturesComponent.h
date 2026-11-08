#pragma once

#include <array>
#include <string>

#include "texture/TextureRegistry.h"

namespace kailux
{
    struct MaterialTexturesComponent
    {
        static constexpr auto kCount{static_cast<uint32_t>(TextureRegistry::kTextureTypes.size())};
        static_assert(kCount == 5, "MaterialTexturesRecord stores exactly one path per texture type");

        std::array<std::string, kCount> paths{};
        std::array<uint32_t, kCount>    revisions{};
        uint32_t                        pendingMask{};

        bool Empty() const
        {
            for (const auto &path : paths)
                if (!path.empty())
                    return false;
            return true;
        }
    };
}