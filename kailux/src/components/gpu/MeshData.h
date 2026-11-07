#pragma once
#include "MeshMaterialData.h"

namespace kailux
{
    using ModelMatrixType = glm::mat4;

    struct MeshData
    {
        ModelMatrixType   modelMatrix{1.f};
        glm::vec4         boundingSphere{};
        glm::vec4         lodErrors{}; // x - LDO1...w - LOD4
        MeshMaterialData  material;
        uint32_t          id{~0u};
        uint32_t          lodCount{1};
        uint32_t          batchIndex{};
        uint32_t          _padding{};
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(MeshData)
}
