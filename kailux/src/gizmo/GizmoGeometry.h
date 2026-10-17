#pragma once
#include "GizmoVertex.h"

namespace kailux::gizmo_geometry
{
    using IndexType = uint32_t;

    struct GizmoData
    {
        std::vector<GizmoVertex> vertices;
        std::vector<IndexType> indices;
    };

    GizmoData generate_point_light_gizmo(glm::vec3 color = {1.f, 0.9f, 0.3f});

    GizmoData generate_camera_gizmo(glm::vec3 color = {1.f, 0.9f, 0.3f});

    namespace details
    {
        void append_segment(GizmoData &data,
                            glm::vec2 a, glm::vec2 b,
                            float halfThickness, glm::vec3 color);
    }
}
