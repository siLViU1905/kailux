#include "Geometry.h"

#include <execution>

namespace kailux
{
    glm::vec4 Geometry::computeBoundingSphere(std::span<const Vertex> vertices)
    {
        if (vertices.empty())
            return {0.f, 0.f, 0.f, 0.f};

        AABB initialAABB;

        auto finalAABB = std::transform_reduce(
            std::execution::par,
            vertices.begin(),
            vertices.end(),
            initialAABB,
            [](const AABB& a, const AABB& b)
            {
                return AABB(
                    glm::min(a.min, b.min),
                    glm::max(a.max, b.max)
                    );
            },
            [](const Vertex& vertex)
            {
                return AABB(
                    vertex.position,
                    vertex.position
                );
            }
            );

        auto center = (finalAABB.min + finalAABB.max) * 0.5f;

        float maxRadiusSq = std::transform_reduce(
            std::execution::par,
            vertices.begin(),
            vertices.end(),
            0.f,
            [](float a, float b)
            {
                return std::max(a, b);
            },
            [center](const Vertex& vertex)
            {
                auto offset = vertex.position - center;
                return glm::dot(offset, offset);
            }
            );

        float radius = std::sqrt(maxRadiusSq);

        return {center, radius};
    }
}
