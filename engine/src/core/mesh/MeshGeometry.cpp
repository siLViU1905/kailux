#include "MeshGeometry.h"

namespace kailux
{
    MeshGeometry::MeshData MeshGeometry::generate_cube()
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

    MeshGeometry::MeshData MeshGeometry::generate_sphere(uint32_t sectors, uint32_t stacks)
    {
        MeshData data;
        data.vertices.reserve((stacks + 1) * (sectors + 1));
        data.indices.reserve(stacks * sectors * 6);

        constexpr float radius = 1.f;

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
}
