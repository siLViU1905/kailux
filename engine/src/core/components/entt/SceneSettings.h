#pragma once

namespace kailux
{
    struct SceneSettings
    {
        glm::vec3 outlineColor{1.f, 0.f, 0.f};

        uint32_t sceneMaxLod{details::kMaxGeometryLods - 1};
        float    sceneLodErrorThreshold{std::numeric_limits<float>::max()};

        uint32_t simulationMaxLod{details::kMaxGeometryLods - 1};
        float    simulationLodErrorThreshold{1.5f};
    };
}