#pragma once
#include "MeshMaterialData.h"
#include "MeshTransformData.h"

namespace kailux
{
    struct MeshData
    {
        ModelMatrixType   modelMatrix{1.f};
        glm::vec4         boundingSphere{};
        MeshMaterialData  material;
        uint32_t          id{~0u};
        std::array<uint32_t, 3> _padding{};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(MeshData)
}
