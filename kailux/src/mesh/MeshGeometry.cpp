#include "MeshGeometry.h"
#include <numbers>
#include <meshoptimizer.h>
#include "../Core.h"

namespace kailux::mesh_geometry
{
    MeshData generate_plane()
    {
        constexpr std::array positions = {
            glm::vec3{-0.5f, 0.f, 0.5f},
            glm::vec3{0.5f, 0.f, 0.5f},
            glm::vec3{0.5f, 0.f, -0.5f},
            glm::vec3{-0.5f, 0.f, -0.5f}
        };
        constexpr std::array uvs = {
            glm::vec2{0, 1},
            glm::vec2{1, 1},
            glm::vec2{1, 0},
            glm::vec2{0, 0}
        };
        MeshData data;
        for (size_t i = 0; i < positions.size(); ++i)
            data.vertices.emplace_back(
                positions[i],
                glm::vec3{0.f, 1.f, 0.f},
                uvs[i],
                glm::vec4(1.f, 0.f, 0.f, 1.f)
            );
        data.indices = {0, 1, 2, 0, 2, 3};
        return data;
    }

    MeshData generate_cube()
    {
        constexpr std::array<std::array<glm::vec3, 4>, 6> positions = {
            {
                {glm::vec3{-1, -1, 1}, glm::vec3{1, -1, 1}, glm::vec3{1, 1, 1}, glm::vec3{-1, 1, 1}},
                {glm::vec3{1, -1, -1}, glm::vec3{-1, -1, -1}, glm::vec3{-1, 1, -1}, glm::vec3{1, 1, -1}},
                {glm::vec3{-1, -1, -1}, glm::vec3{1, -1, -1}, glm::vec3{1, -1, 1}, glm::vec3{-1, -1, 1}},
                {glm::vec3{-1, 1, 1}, glm::vec3{1, 1, 1}, glm::vec3{1, 1, -1}, glm::vec3{-1, 1, -1}},
                {glm::vec3{-1, -1, -1}, glm::vec3{-1, -1, 1}, glm::vec3{-1, 1, 1}, glm::vec3{-1, 1, -1}},
                {glm::vec3{1, -1, 1}, glm::vec3{1, -1, -1}, glm::vec3{1, 1, -1}, glm::vec3{1, 1, 1}},
            }
        };
        constexpr std::array normals = {
            glm::vec3{0, 0, 1},
            glm::vec3{0, 0, -1},
            glm::vec3{0, -1, 0},
            glm::vec3{0, 1, 0},
            glm::vec3{-1, 0, 0},
            glm::vec3{1, 0, 0}
        };
        constexpr std::array uvs = {
            glm::vec2{0, 1},
            glm::vec2{1, 1},
            glm::vec2{1, 0},
            glm::vec2{0, 0}
        };

        MeshData data;

        for (int f = 0; f < 6; f++)
        {
            auto base = static_cast<uint32_t>(data.vertices.size());
            for (int i = 0; i < 4; i++)
                data.vertices.emplace_back(
                    positions[f][i] * 0.5f,
                    normals[f],
                    uvs[i],
                    glm::vec4(1.f, 0.f, 0.f, 1.f)
                );

            data.indices.insert(data.indices.end(), {
                                    base + 0, base + 1, base + 2,
                                    base + 0, base + 2, base + 3
                                });
        }
        return data;
    }

    MeshData generate_sphere(uint32_t sectors, uint32_t stacks)
    {
        MeshData data;
        data.vertices.reserve((stacks + 1) * (sectors + 1));
        data.indices.reserve(stacks * sectors * 6);

        constexpr float radius = 0.5f;

        const float sectorStep = 2.f * std::numbers::pi_v<float> / static_cast<float>(sectors);
        const float stackStep = std::numbers::pi_v<float> / static_cast<float>(stacks);
        constexpr float lengthInv = 1.f / radius;

        for (uint32_t i = 0; i <= stacks; ++i)
        {
            float stackAngle = std::numbers::pi_v<float> / 2.f - static_cast<float>(i) * stackStep;
            float xy = radius * std::cos(stackAngle);
            float z = radius * std::sin(stackAngle);

            for (uint32_t j = 0; j <= sectors; ++j)
            {
                float sectorAngle = static_cast<float>(j) * sectorStep;

                float x = xy * std::cos(sectorAngle);
                float y = xy * std::sin(sectorAngle);

                glm::vec3 pos(x, y, z);
                glm::vec3 normal = pos * lengthInv;
                glm::vec2 uv(
                    static_cast<float>(j) / static_cast<float>(sectors),
                    static_cast<float>(i) / static_cast<float>(stacks)
                );
                glm::vec4 tangent{-std::sin(sectorAngle), std::cos(sectorAngle), 0.f, 1.f};

                data.vertices.emplace_back(pos, normal, uv, tangent);
            }
        }

        for (uint32_t i = 0; i < stacks; ++i)
        {
            uint32_t k1 = i * (sectors + 1);
            uint32_t k2 = k1 + sectors + 1;

            for (uint32_t j = 0; j < sectors; ++j, ++k1, ++k2)
            {
                if (i != 0)
                {
                    data.indices.push_back(k1);
                    data.indices.push_back(k2);
                    data.indices.push_back(k1 + 1);
                }

                if (i != (stacks - 1))
                {
                    data.indices.push_back(k1 + 1);
                    data.indices.push_back(k2);
                    data.indices.push_back(k2 + 1);
                }
            }
        }

        return data;
    }

    MeshData generate_cone(uint32_t segments)
    {
        MeshData data;
        data.vertices.reserve(2 * (segments + 1) + segments + 2);
        data.indices.reserve(segments * 6);

        constexpr float radius{0.5f};
        constexpr float height{1.f};
        constexpr float halfHeight{height * 0.5f};

        const auto step{2.f * std::numbers::pi_v<float> / static_cast<float>(segments)};

        const auto slope{glm::normalize(glm::vec2{height, radius})};

        const auto sideNormal{
            [&](float angle)
            {
                return glm::vec3{slope.x * std::cos(angle), slope.y, slope.x * std::sin(angle)};
            }
        };

        for (uint32_t j{}; j <= segments; ++j)
        {
            const auto angle{(static_cast<float>(j) + 0.5f) * step};

            data.vertices.emplace_back(
                glm::vec3{0.f, halfHeight, 0.f},
                sideNormal(angle),
                glm::vec2{(static_cast<float>(j) + 0.5f) / static_cast<float>(segments), 0.f},
                glm::vec4{-std::sin(angle), 0.f, std::cos(angle), 1.f}
            );
        }

        for (uint32_t j{}; j <= segments; ++j)
        {
            const auto angle{static_cast<float>(j) * step};
            const auto c{std::cos(angle)};
            const auto s{std::sin(angle)};

            data.vertices.emplace_back(
                glm::vec3{radius * c, -halfHeight, radius * s},
                sideNormal(angle),
                glm::vec2{static_cast<float>(j) / static_cast<float>(segments), 1.f},
                glm::vec4{-s, 0.f, c, 1.f}
            );
        }

        for (uint32_t j{}; j < segments; ++j)
        {
            const auto apex{j};
            const auto k2{segments + 1 + j};

            data.indices.insert(data.indices.end(), {apex, k2 + 1, k2});
        }

        details::append_cap(data, segments, radius, -halfHeight, false);

        return data;
    }

    MeshData generate_cylinder(uint32_t segments)
    {
        MeshData data;
        data.vertices.reserve(2 * (segments + 1) + 2 * (segments + 2));
        data.indices.reserve(segments * 12);

        constexpr float radius{0.5f};
        constexpr float halfHeight{0.5f};

        const auto step{2.f * std::numbers::pi_v<float> / static_cast<float>(segments)};

        for (uint32_t i{}; i <= 1; ++i)
        {
            const auto y{i == 0 ? halfHeight : -halfHeight};

            for (uint32_t j{}; j <= segments; ++j)
            {
                const auto angle{static_cast<float>(j) * step};
                const auto c{std::cos(angle)};
                const auto s{std::sin(angle)};

                data.vertices.emplace_back(
                    glm::vec3{radius * c, y, radius * s},
                    glm::vec3{c, 0.f, s},
                    glm::vec2{static_cast<float>(j) / static_cast<float>(segments), static_cast<float>(i)},
                    glm::vec4{-s, 0.f, c, 1.f}
                );
            }
        }
        details::append_grid_indices(data, 0, 1, segments);

        details::append_cap(data, segments, radius, halfHeight, true);
        details::append_cap(data, segments, radius, -halfHeight, false);

        return data;
    }

    MeshData generate_torus(float ringRadius, float tubeRadius, uint32_t ringSegments, uint32_t tubeSegments)
    {
        MeshData data;
        data.vertices.reserve((ringSegments + 1) * (tubeSegments + 1));
        data.indices.reserve(ringSegments * tubeSegments * 6);

        const auto ringStep{2.f * std::numbers::pi_v<float> / static_cast<float>(ringSegments)};
        const auto tubeStep{2.f * std::numbers::pi_v<float> / static_cast<float>(tubeSegments)};

        for (uint32_t i{}; i <= tubeSegments; ++i)
        {
            const auto tubeAngle{std::numbers::pi_v<float> / 2.f - static_cast<float>(i) * tubeStep};
            const auto tc{std::cos(tubeAngle)};
            const auto ts{std::sin(tubeAngle)};

            for (uint32_t j{}; j <= ringSegments; ++j)
            {
                const auto ringAngle{static_cast<float>(j) * ringStep};
                const auto rc{std::cos(ringAngle)};
                const auto rs{std::sin(ringAngle)};

                const auto distance{ringRadius + tubeRadius * tc};

                data.vertices.emplace_back(
                    glm::vec3{distance * rc, tubeRadius * ts, distance * rs},
                    glm::vec3{tc * rc, ts, tc * rs},
                    glm::vec2{
                        static_cast<float>(j) / static_cast<float>(ringSegments),
                        static_cast<float>(i) / static_cast<float>(tubeSegments)
                    },
                    glm::vec4{-rs, 0.f, rc, 1.f}
                );
            }
        }
        details::append_grid_indices(data, 0, tubeSegments, ringSegments);

        return data;
    }

    glm::vec4 compute_bounding_sphere(std::span<const Vertex> vertices)
    {
        if (vertices.empty())
            return {0.f, 0.f, 0.f, 0.f};

        glm::vec3 min{vertices[0].position};
        glm::vec3 max{vertices[0].position};

        for (const auto& vertex : vertices)
        {
            min = glm::min(min, vertex.position);
            max = glm::max(max, vertex.position);
        }

        const auto center{(min + max) * 0.5f};

        float maxRadiusSq{};
        for (const auto& vertex : vertices)
        {
            const auto offset{vertex.position - center};
            maxRadiusSq = std::max(maxRadiusSq, glm::dot(offset, offset));
        }

        const float radius{std::sqrt(maxRadiusSq)};
        return {center, radius};
    }

    void optimize_mesh(MeshData &meshData)
    {
        auto& vertices{meshData.vertices};
        auto& indices{meshData.indices};

        if (vertices.empty() || indices.empty())
            return;

        const auto indexCount{indices.size()};

        std::vector<uint32_t> remap(indexCount);
        const auto vertexCount{
            meshopt_generateVertexRemap(
                remap.data(),
                indices.data(),
                indexCount,
                vertices.data(),
                vertices.size(),
                sizeof(Vertex)
            )
        };

        std::vector<Vertex> newVertices(vertexCount);
        std::vector<IndexType> newIndices(indexCount);

        meshopt_remapIndexBuffer(
            newIndices.data(),
            indices.data(),
            indexCount,
            remap.data()
        );
        meshopt_remapVertexBuffer(
            newVertices.data(),
            vertices.data(),
            vertices.size(),
            sizeof(Vertex), remap.data()
        );

        vertices = std::move(newVertices);
        indices = std::move(newIndices);

        meshopt_optimizeVertexCache(
            indices.data(),
            indices.data(),
            indexCount,
            vertexCount
        );

        meshopt_optimizeOverdraw(
            indices.data(),
            indices.data(),
            indexCount,
            &vertices.front().position.x,
            vertexCount,
            sizeof(Vertex),
            1.05f
        );

        meshopt_optimizeVertexFetch(
            vertices.data(),
            indices.data(),
            indexCount,
            vertices.data(),
            vertexCount,
            sizeof(Vertex)
        );
    }

    void generate_lods(MeshData &meshData)
    {
        meshData.lods.clear();

        const auto &vertices{meshData.vertices};
        if (vertices.empty() || meshData.indices.size() < kailux::details::kMinLodIndices)
            return;

        const float meshScale{
            meshopt_simplifyScale(
                &vertices.front().position.x,
                vertices.size(),
                sizeof(Vertex)
            )
        };

        constexpr std::array<float, kailux::details::kMaxGeometryLods - 1> kTargets{0.25f, 0.0625f, 0.015f, 0.004f};
        constexpr uint32_t kMinIndices{kailux::details::kMinLodIndices / 2};

        std::vector src{meshData.indices};
        const auto originalSize{static_cast<uint32_t>(meshData.indices.size())};

        uint32_t level{};
        float carriedError{};

        while (level < kTargets.size())
        {
            const auto wanted{static_cast<uint32_t>(originalSize * kTargets[level]) / 3 * 3};
            if (wanted < kMinIndices)
                break;

            const uint32_t step{std::max(wanted, static_cast<uint32_t>(src.size()) / 2 / 3 * 3)};

            std::vector<IndexType> dst(src.size());
            float error{};

            const auto count{
                meshopt_simplify(
                    dst.data(),
                    src.data(),
                    src.size(),
                    &vertices.front().position.x,
                    vertices.size(),
                    sizeof(Vertex),
                    step,
                    0.1f,
                    meshopt_SimplifyLockBorder,
                    &error
                )
            };
            if (count >= src.size() * 0.95f)
                break;

            dst.resize(count);
            src = std::move(dst);
            carriedError = std::max(carriedError, error);

            if (src.size() <= wanted)
            {
                std::vector lodIndices{src};
                meshopt_optimizeVertexCache(
                    lodIndices.data(),
                    lodIndices.data(),
                    lodIndices.size(),
                    vertices.size()
                );
                meshData.lods.emplace_back(std::move(lodIndices), carriedError * meshScale);
                ++level;
            }
        }
    }

    namespace details
    {
        const glm::vec4& plane_bounds()
        {
            static const auto bounds{compute_bounding_sphere(generate_plane().vertices)};
            return bounds;
        }

        const glm::vec4& cube_bounds()
        {
            static const auto bounds{compute_bounding_sphere(generate_cube().vertices)};
            return bounds;
        }

        const glm::vec4& sphere_bounds()
        {
            static const auto bounds{compute_bounding_sphere(generate_sphere().vertices)};
            return bounds;
        }

        const glm::vec4& cone_bounds()
        {
            static const auto bounds{compute_bounding_sphere(generate_cone().vertices)};
            return bounds;
        }

        const glm::vec4& cylinder_bounds()
        {
            static const auto bounds{compute_bounding_sphere(generate_cylinder().vertices)};
            return bounds;
        }

        const glm::vec4& torus_bounds()
        {
            static const auto bounds{compute_bounding_sphere(generate_torus().vertices)};
            return bounds;
        }

        void append_cap(MeshData &data, uint32_t segments, float radius, float y, bool up)
        {
            const auto step{2.f * std::numbers::pi_v<float> / static_cast<float>(segments)};
            const glm::vec3 normal{0.f, up ? 1.f : -1.f, 0.f};
            constexpr glm::vec4 tangent{1.f, 0.f, 0.f, 1.f};

            const auto center{static_cast<uint32_t>(data.vertices.size())};
            data.vertices.emplace_back(glm::vec3{0.f, y, 0.f}, normal, glm::vec2{0.5f, 0.5f}, tangent);

            for (uint32_t j{}; j <= segments; ++j)
            {
                const auto angle{static_cast<float>(j) * step};
                const auto c{std::cos(angle)};
                const auto s{std::sin(angle)};

                data.vertices.emplace_back(
                    glm::vec3{radius * c, y, radius * s},
                    normal,
                    glm::vec2{0.5f + 0.5f * c, 0.5f + (up ? -0.5f : 0.5f) * s},
                    tangent
                );
            }

            for (uint32_t j{}; j < segments; ++j)
            {
                const auto a{center + 1 + j};
                const auto b{a + 1};
                if (up)
                    data.indices.insert(data.indices.end(), {center, b, a});
                else
                    data.indices.insert(data.indices.end(), {center, a, b});
            }
        }

        void append_grid_indices(MeshData &data, uint32_t base, uint32_t rows, uint32_t columns)
        {
            for (uint32_t i{}; i < rows; ++i)
                for (uint32_t j{}; j < columns; ++j)
                {
                    const auto k1{base + i * (columns + 1) + j};
                    const auto k2{k1 + columns + 1};

                    data.indices.insert(data.indices.end(), {
                                            k1, k1 + 1, k2,
                                            k1 + 1, k2 + 1, k2
                                        });
                }
        }
    }
}
