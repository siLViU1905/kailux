#pragma once
#include "core/Core.h"

namespace kailux
{
    struct MeshMaterialData
    {
        glm::vec4 albedoAndRoughness{0.75f, 0.75f, 0.75f, 0.5f};
        glm::vec4 pbrParams{0.5f, 0.5f, 0.f,0.f};
        uint32_t  albedoIdx{};
        uint32_t  normalIdx{};
        uint32_t  roughnessIdx{};
        uint32_t  metallicIdx{};
        uint32_t  aoIdx{};
        std::array<uint32_t, 3> _padding{};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(MeshMaterialData)
}
