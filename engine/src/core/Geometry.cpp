#include "Geometry.h"

namespace kailux
{
    glm::vec4 Geometry::computeBoundingSphere(std::span<const Vertex> vertices)
    {
        if (vertices.empty())
            return {0.f, 0.f, 0.f, 0.f};

        glm::vec3 min = vertices[0].position;
        glm::vec3 max = vertices[0].position;

        for (const auto& vertex : vertices)
        {
            min = glm::min(min, vertex.position);
            max = glm::max(max, vertex.position);
        }

        auto center = (min + max) * 0.5f;

        float maxRadiusSq = 0.f;
        for (const auto& vertex : vertices)
        {
            auto offset = vertex.position - center;
            maxRadiusSq = std::max(maxRadiusSq, glm::dot(offset, offset));
        }

        float radius = std::sqrt(maxRadiusSq);
        return {center, radius};
    }
}
