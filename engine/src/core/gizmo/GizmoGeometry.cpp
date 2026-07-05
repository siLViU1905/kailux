#include "GizmoGeometry.h"

namespace kailux
{
    GizmoGeometry::GizmoData GizmoGeometry::generate_point_light_gizmo(glm::vec3 color)
    {
        GizmoData data;

        constexpr float pi = std::numbers::pi_v<float>;

        constexpr uint32_t circleSegments = 24;
        constexpr float    circleRadius   = 0.35f;
        constexpr float    thickness      = 0.02f;

        constexpr uint32_t rayCount = 8;
        constexpr float    rayInner = 0.5f;
        constexpr float    rayOuter = 0.75f;

        constexpr float halfThickness = thickness / 2.f;

        constexpr uint32_t quadCount = circleSegments + rayCount;
        data.vertices.reserve(quadCount * 4);
        data.indices.reserve(quadCount * 6);

        for (uint32_t i = 0; i < circleSegments; ++i)
        {
            float a0 = (static_cast<float>(i)     / circleSegments) * 2.f * pi;
            float a1 = (static_cast<float>(i + 1) / circleSegments) * 2.f * pi;

            glm::vec2 p0 = {std::cos(a0) * circleRadius, std::sin(a0) * circleRadius};
            glm::vec2 p1 = {std::cos(a1) * circleRadius, std::sin(a1) * circleRadius};

            append_segment(data, p0, p1, halfThickness, color);
        }

        for (uint32_t i = 0; i < rayCount; ++i)
        {
            float angle = (static_cast<float>(i) / rayCount) * 2.f * pi;
            glm::vec2 dir = {std::cos(angle), std::sin(angle)};

            glm::vec2 start = dir * rayInner;
            glm::vec2 end   = dir * rayOuter;

            append_segment(data, start, end, halfThickness, color);
        }

        return data;
    }

    void GizmoGeometry::append_segment(GizmoData &data, glm::vec2 a, glm::vec2 b, float halfThickness, glm::vec3 color)
    {
        auto dir = b - a;
        float len = glm::length(dir);
        if (len < 1e-6f)
            return;
        dir /= len;

        glm::vec2 perp = {-dir.y, dir.x};
        glm::vec2 offset = perp * halfThickness;

        auto base = static_cast<IndexType>(data.vertices.size());

        data.vertices.emplace_back(glm::vec3{a.x + offset.x, a.y + offset.y, 0.f});
        data.vertices.emplace_back(glm::vec3{a.x - offset.x, a.y - offset.y, 0.f});
        data.vertices.emplace_back(glm::vec3{b.x - offset.x, b.y - offset.y, 0.f});
        data.vertices.emplace_back(glm::vec3{b.x + offset.x, b.y + offset.y, 0.f});

        data.indices.push_back(base + 0);
        data.indices.push_back(base + 1);
        data.indices.push_back(base + 2);

        data.indices.push_back(base + 0);
        data.indices.push_back(base + 2);
        data.indices.push_back(base + 3);

    }
}
