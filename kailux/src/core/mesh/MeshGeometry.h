#pragma once
#include "Vertex.h"

namespace kailux
{
    class MeshGeometry
    {
    public:
        using IndexType = uint32_t;
        struct LodLevel
        {
            std::vector<IndexType> indices;
            float                  error{};
        };
        struct MeshData
        {
            std::vector<Vertex>    vertices;
            std::vector<IndexType> indices;
            std::vector<LodLevel>  lods;
        };

        static MeshData generate_cube();
        static MeshData generate_sphere(uint32_t sectors = 32, uint32_t stacks = 32);

        static void optimize_mesh(MeshData& meshData);
        static void generate_lods(MeshData& meshData);
    };
}
