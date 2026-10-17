#include "PointShadows.h"

namespace kailux
{
    float PointShadows::score(const PointLightData &light, const glm::vec3 &cameraPos)
    {
        if (light.colorAndEnabled.w < 0.5f)
            return 0.f;

        const glm::vec3 position{light.positionAndIntensity};
        const float intensity{light.positionAndIntensity.w};
        const float range{light.range.x};

        if (range <= 0.f || intensity <= 0.f)
            return 0.f;

        const float distance{glm::length(position - cameraPos)};
        if (distance > range)
            return 0.f;

        const float attenuation{1.f / std::max(distance * distance, 0.0001f)};
        float rangeFade{std::clamp(1.f - std::pow(distance / range, 4.f), 0.f, 1.f)};
        rangeFade *= rangeFade;

        return intensity * attenuation * rangeFade * range;
    }

    PointShadowSetup PointShadows::build(const PointLightData &light, uint32_t lightIndex, float nearPlane)
    {
        PointShadowSetup setup;
        setup.position = glm::vec3(light.positionAndIntensity);
        setup.nearPlane = nearPlane;
        setup.farPlane = std::max(light.range.x, nearPlane + 0.01f);
        setup.lightIndex = lightIndex;
        setup.faceViewProjections = build_faces(setup.position, setup.nearPlane, setup.farPlane);
        return setup;

    }

    std::array<glm::mat4, details::kPointShadowFaceCount> PointShadows::build_faces(
        const glm::vec3 &position,
        float nearPlane,
        float farPlane
    )
    {
        const auto projection{glm::perspectiveRH_ZO(glm::radians(90.f), 1.f, nearPlane, farPlane)};

        constexpr std::array targets{
            glm::vec3{1.f, 0.f, 0.f},
            glm::vec3{-1.f, 0.f, 0.f},
            glm::vec3{0.f, 1.f, 0.f},
            glm::vec3{0.f, -1.f, 0.f},
            glm::vec3{0.f, 0.f, 1.f},
            glm::vec3{0.f, 0.f, -1.f}
        };

        constexpr std::array ups{
            glm::vec3{0.f, -1.f, 0.f},
            glm::vec3{0.f, -1.f, 0.f},
            glm::vec3{0.f, 0.f, 1.f},
            glm::vec3{0.f, 0.f, -1.f},
            glm::vec3{0.f, -1.f, 0.f},
            glm::vec3{0.f, -1.f, 0.f}
        };

        std::array<glm::mat4, details::kPointShadowFaceCount> faces{};
        for (uint32_t face{}; face < details::kPointShadowFaceCount; ++face)
            faces[face] = projection * glm::lookAt(position, position + targets[face], ups[face]);

        return faces;
    }
}
