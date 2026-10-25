#include "Bounds.h"

namespace kailux::bounds
{
    Sphere world_sphere(const glm::mat4 &model, const glm::vec4 &localSphere)
    {
        const float maxScale{std::max({
            glm::length(glm::vec3{model[0]}),
            glm::length(glm::vec3{model[1]}),
            glm::length(glm::vec3{model[2]})
        })};

        return {
            glm::vec3{model * glm::vec4{glm::vec3{localSphere}, 1.f}},
            localSphere.w * maxScale
        };
    }

    bool sphere_in_planes(const std::array<glm::vec4, 6> &planes, const Sphere &sphere, bool skipNear)
    {
        constexpr uint32_t kNearPlane{4};
        for (uint32_t i{}; i < planes.size(); ++i)
        {
            if (skipNear && i == kNearPlane)
                continue;

            if (glm::dot(glm::vec3{planes[i]}, sphere.center) + planes[i].w < -sphere.radius)
                return false;
        }
        return true;
    }
}
