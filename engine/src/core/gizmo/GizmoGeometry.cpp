#include "GizmoGeometry.h"

namespace kailux
{
    GizmoGeometry::GizmoData GizmoGeometry::generate_point_light_gizmo(glm::vec3 color)
    {
        GizmoData data;

        constexpr float pi{std::numbers::pi_v<float>};

        constexpr uint32_t circleSegments{24};
        constexpr float    circleRadius{0.35f};
        constexpr float    thickness{0.02f};

        constexpr uint32_t rayCount{8};
        constexpr float    rayInner{0.5f};
        constexpr float    rayOuter{0.75f};

        constexpr float halfThickness{thickness / 2.f};

        constexpr uint32_t quadCount{circleSegments + rayCount};
        data.vertices.reserve(quadCount * 4);
        data.indices.reserve(quadCount * 6);

        for (uint32_t i{0}; i < circleSegments; ++i)
        {
            float a0{(static_cast<float>(i)     / circleSegments) * 2.f * pi};
            float a1{(static_cast<float>(i + 1) / circleSegments) * 2.f * pi};

            glm::vec2 p0{std::cos(a0) * circleRadius, std::sin(a0) * circleRadius};
            glm::vec2 p1{std::cos(a1) * circleRadius, std::sin(a1) * circleRadius};

            append_segment(data, p0, p1, halfThickness, color);
        }

        for (uint32_t i{}; i < rayCount; ++i)
        {
            const float angle{(static_cast<float>(i) / rayCount) * 2.f * pi};
            glm::vec2 dir{std::cos(angle), std::sin(angle)};

            const auto start{dir * rayInner};
            const auto end{dir * rayOuter};

            append_segment(data, start, end, halfThickness, color);
        }

        return data;
    }

    GizmoGeometry::GizmoData GizmoGeometry::generate_camera_gizmo(glm::vec3 color)
    {
        GizmoData data;

        constexpr float thickness{0.02f};
        constexpr float halfThickness{thickness / 2.f};

        constexpr float bodyMinX{-0.55f};
        constexpr float bodyMaxX{0.05f};
        constexpr float bodyHalfY{0.30f};

        constexpr float lensX{0.55f};
        constexpr float lensNearY{0.16f};
        constexpr float lensFarY{0.40f};

        constexpr uint32_t bodyPoints{4};
        constexpr uint32_t lensPoints{4};

        constexpr std::array body{
            glm::vec2{bodyMinX, -bodyHalfY},
            glm::vec2{bodyMaxX, -bodyHalfY},
            glm::vec2{bodyMaxX,  bodyHalfY},
            glm::vec2{bodyMinX,  bodyHalfY},
        };

        constexpr std::array lens{
            glm::vec2{bodyMaxX, -lensNearY},
            glm::vec2{lensX,    -lensFarY },
            glm::vec2{lensX,     lensFarY },
            glm::vec2{bodyMaxX,  lensNearY},
        };

        constexpr uint32_t quadCount{bodyPoints + (lensPoints - 1)};
        data.vertices.reserve(quadCount * 4);
        data.indices.reserve(quadCount * 6);

        for (uint32_t i{}; i < bodyPoints; ++i)
            append_segment(data, body[i], body[(i + 1) % bodyPoints], halfThickness, color);

        for (uint32_t i{}; i + 1 < lensPoints; ++i)
            append_segment(data, lens[i], lens[i + 1], halfThickness, color);

        return data;
    }

    void GizmoGeometry::append_segment(GizmoData &data, glm::vec2 a, glm::vec2 b, float halfThickness, glm::vec3 color)
    {
        auto dir{b - a};
        const float len{glm::length(dir)};
        if (len < 1e-6f)
            return;
        dir /= len;

        const glm::vec2 perp{-dir.y, dir.x};
        const glm::vec2 offset{perp * halfThickness};

        const auto base{static_cast<IndexType>(data.vertices.size())};

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
