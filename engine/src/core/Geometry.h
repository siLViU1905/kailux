#pragma once
#include "mesh/Vertex.h"

namespace kailux
{
    class Geometry
    {
    public:
        static glm::vec4 computeBoundingSphere(std::span<const Vertex> vertices);

    private:
        struct AABB
        {
            glm::vec3 min{std::numeric_limits<float>::max()};
            glm::vec3 max{std::numeric_limits<float>::lowest()};
        };
    };
}
