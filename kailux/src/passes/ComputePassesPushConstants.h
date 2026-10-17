#pragma once

namespace kailux
{
    struct ComputePassesPushConstants
    {
        struct MouseCords
        {
            uint32_t x{};
            uint32_t y{};
        };

        struct CullParams
        {
            std::array<glm::vec4, 6> frustumPlanes{};
            glm::vec4                cameraPosition{};
            glm::vec4 params{
                0.f, // x - total objects
                kUnrestrictedLod, // y - max lod
                kSceneLodErrorThreshold, // z - lod error threshold
                0.f
            };

            static constexpr auto kUnrestrictedLod{static_cast<float>(details::kMaxGeometryLods - 1)};

            static constexpr auto kSceneLodErrorThreshold{std::numeric_limits<float>::max()};
            static constexpr auto kSimulationLodErrorThreshold{1.5f};
        };
    };
}
