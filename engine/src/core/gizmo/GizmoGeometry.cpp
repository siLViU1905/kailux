#include "GizmoGeometry.h"
#include <numbers>

namespace kailux
{
    GizmoGeometry::GizmoData GizmoGeometry::generate_point_light_gizmo(glm::vec3 color)
    {
        GizmoData data;

        constexpr float pi{std::numbers::pi_v<float>};

        constexpr float thickness{0.02f};
        constexpr float halfThickness{thickness / 2.f};

        constexpr uint32_t globeSegments{20};
        constexpr float    globeRadius{0.30f};
        constexpr float    globeOpenHalfAngle{pi / 6.f};

        constexpr float neckTopHalfWidth{0.15f};
        constexpr float neckTopY{-0.26f};
        constexpr float baseHalfWidth{0.11f};
        constexpr float baseTopY{-0.40f};
        constexpr float baseBottomY{-0.58f};
        constexpr uint32_t baseBands{4};
        constexpr float tipHalfWidth{0.05f};
        constexpr float tipY{-0.64f};

        constexpr uint32_t rayCount{8};
        constexpr float    rayInner{0.42f};
        constexpr float    rayOuter{0.64f};

        constexpr float filamentPostX{0.05f};
        constexpr float filamentLowY{-0.12f};
        constexpr float filamentHighY{-0.05f};

        constexpr uint32_t quadCount{globeSegments + 2 + 2 + baseBands + 3 + 2 + 4 + rayCount};
        data.vertices.reserve(quadCount * 4);
        data.indices.reserve(quadCount * 6);

        auto polyline = [&](auto const &points)
        {
            for (size_t i{}; i + 1 < points.size(); ++i)
                append_segment(data, points[i], points[i + 1], halfThickness, color);
        };

        {
            const float start{-pi / 2.f + globeOpenHalfAngle};
            const float sweep{2.f * pi - 2.f * globeOpenHalfAngle};

            for (uint32_t i{}; i < globeSegments; ++i)
            {
                const float a0{start + (static_cast<float>(i)     / globeSegments) * sweep};
                const float a1{start + (static_cast<float>(i + 1) / globeSegments) * sweep};

                const glm::vec2 p0{std::cos(a0) * globeRadius, std::sin(a0) * globeRadius};
                const glm::vec2 p1{std::cos(a1) * globeRadius, std::sin(a1) * globeRadius};

                append_segment(data, p0, p1, halfThickness, color);
            }
        }

        polyline(std::array{
            glm::vec2{ neckTopHalfWidth, neckTopY},
            glm::vec2{ baseHalfWidth,    baseTopY},
            glm::vec2{ baseHalfWidth,    baseBottomY},
        });
        polyline(std::array{
            glm::vec2{-neckTopHalfWidth, neckTopY},
            glm::vec2{-baseHalfWidth,    baseTopY},
            glm::vec2{-baseHalfWidth,    baseBottomY},
        });

        for (uint32_t i{}; i < baseBands; ++i)
        {
            const float t{static_cast<float>(i) / static_cast<float>(baseBands - 1)};
            const float y{baseTopY + (baseBottomY - baseTopY) * t};
            append_segment(data, {-baseHalfWidth, y}, {baseHalfWidth, y}, halfThickness, color);
        }

        polyline(std::array{
            glm::vec2{-baseHalfWidth, baseBottomY},
            glm::vec2{-tipHalfWidth,  tipY},
            glm::vec2{ tipHalfWidth,  tipY},
            glm::vec2{ baseHalfWidth, baseBottomY},
        });

        append_segment(data, {-filamentPostX, baseTopY}, {-filamentPostX, filamentLowY}, halfThickness, color);
        append_segment(data, { filamentPostX, baseTopY}, { filamentPostX, filamentLowY}, halfThickness, color);
        polyline(std::array{
            glm::vec2{-filamentPostX,        filamentLowY},
            glm::vec2{-filamentPostX * 0.5f, filamentHighY},
            glm::vec2{ 0.f,                  filamentLowY},
            glm::vec2{ filamentPostX * 0.5f, filamentHighY},
            glm::vec2{ filamentPostX,        filamentLowY},
        });

        for (uint32_t i{}; i < rayCount; ++i)
        {
            const float angle{(static_cast<float>(i) / rayCount) * 2.f * pi};
            const glm::vec2 dir{std::cos(angle), std::sin(angle)};

            if (dir.y < -0.9f)
                continue;

            append_segment(data, dir * rayInner, dir * rayOuter, halfThickness, color);
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
