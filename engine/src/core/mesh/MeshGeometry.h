#pragma once
#include "Vertex.h"

namespace kailux
{
    class MeshGeometry
    {
    public:
        using IndexType = uint32_t;
        struct MeshData
        {
            std::vector<Vertex> vertices;
            std::vector<IndexType> indices;
        };

        static MeshData generate_cube();
        static MeshData generate_sphere(uint32_t sectors = 32, uint32_t stacks = 32);
    };
}
