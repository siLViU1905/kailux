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

    // 1 x 1 quad on XZ
    MeshData generate_plane();

    MeshData generate_cube();

    MeshData generate_sphere(uint32_t sectors = 32, uint32_t stacks = 32);

    MeshData generate_cone(uint32_t segments = 32);

    MeshData generate_cylinder(uint32_t segments = 32);

    MeshData generate_torus(float ringRadius = 0.35f,
                            float tubeRadius = 0.15f,
                            uint32_t ringSegments = 48,
                            uint32_t tubeSegments = 24
    );

    glm::vec4 compute_bounding_sphere(std::span<const Vertex> vertices);

    void optimize_mesh(MeshData &meshData);

    void generate_lods(MeshData &meshData);

    namespace details
    {
        const glm::vec4& plane_bounds();
        const glm::vec4& cube_bounds();
        const glm::vec4& sphere_bounds();
        const glm::vec4& cone_bounds();
        const glm::vec4& cylinder_bounds();
        const glm::vec4& torus_bounds();

        void append_cap(MeshData &data, uint32_t segments, float radius, float y, bool up);
        void append_grid_indices(MeshData &data, uint32_t base, uint32_t rows, uint32_t columns);
    }
}
