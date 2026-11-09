#pragma once
#include "Core.h"

namespace kailux
{
    struct MeshMaterialData
    {
        glm::vec4 albedoAndRoughness{0.75f, 0.75f, 0.75f, 1.f};
        glm::vec4 pbrParams{0.f, 1.f, 0.f,0.f}; // x - metallic y - ao z - grid cell(0 = no grid)
        uint32_t  materialIdx{};
        std::array<uint32_t, 3> _padding{};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(MeshMaterialData)

    constexpr float kDefaultGridCellSize{1.f};

    constexpr MeshMaterialData make_grid_material(float cellSize = kDefaultGridCellSize)
    {
        MeshMaterialData material;
        material.pbrParams.z = cellSize;
        return material;
    }
}
