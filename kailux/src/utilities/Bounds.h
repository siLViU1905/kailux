#pragma once
#include <algorithm>
#include <array>
#include <glm/glm.hpp>

namespace kailux::bounds
{
    struct Sphere
    {
        glm::vec3 center;
        float radius;
    };

    Sphere world_sphere(const glm::mat4& model, const glm::vec4& localSphere);

    bool sphere_in_planes(const std::array<glm::vec4, 6> &planes, const Sphere &sphere, bool skipNear = false);
}