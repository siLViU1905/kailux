#pragma once
#include "core/Core.h"

namespace kailux
{
    struct MeshMaterialData
    {
        glm::vec4 albedoAndRoughness{0.75f, 0.75f, 0.75f, 0.5f};
        glm::vec4 pbrParams{0.5f, 0.5f, 0.f,0.f};
        uint32_t  materialIdx{};
        std::array<uint32_t, 3> _padding{};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(MeshMaterialData)
}
