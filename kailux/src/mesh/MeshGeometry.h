#pragma once
#include "Vertex.h"

namespace kailux::mesh_geometry
{
    using IndexType = uint32_t;

    struct LodLevel
    {
        std::vector<IndexType> indices;
        float error{};
    };

    struct MeshData
    {
        std::vector<Vertex> vertices;
        std::vector<IndexType> indices;
        std::vector<LodLevel> lods;
    };

    MeshData generate_cube();

    MeshData generate_sphere(uint32_t sectors = 32, uint32_t stacks = 32);

    void optimize_mesh(MeshData &meshData);

    void generate_lods(MeshData &meshData);
}
