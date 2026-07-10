#pragma once
#include "core/Core.h"

namespace kailux
{
    struct MaterialSlot
    {
        uint32_t albedoIdx{~0u};
        uint32_t normalIdx{~0u};
        uint32_t roughnessIdx{~0u};
        uint32_t metallicIdx{~0u};
        uint32_t aoIdx{~0u};
        std::array<uint32_t, 3> _padding{};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(MaterialSlot)
}
