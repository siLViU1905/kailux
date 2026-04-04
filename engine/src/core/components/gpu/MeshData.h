#pragma once
#include "MeshMaterialData.h"
#include "MeshTransformData.h"

namespace kailux
{
    struct MeshData
    {
        ModelMatrixType   modelMatrix{1.f};
        MeshMaterialData  material;
    };

    KAILUX_CHECK_DATA_STRUCTURE_SIZE(MeshData)
}
