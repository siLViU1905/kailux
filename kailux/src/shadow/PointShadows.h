#pragma once
#include "../Core.h"
#include "components/gpu/PointLightData.h"

namespace kailux
{
    struct PointShadowSetup
    {
        std::array<glm::mat4, details::kPointShadowFaceCount> faceViewProjections{1.f};

        glm::vec3 position{};
        float     nearPlane{0.05f};
        float     farPlane{10.0f};

        uint32_t  lightIndex{~0u};

        constexpr bool Valid() const
        {
            return lightIndex != ~0u;
        }
    };

    namespace point_shadow
    {
        constexpr float kStickiness{1.25f};

        float score(const PointLightData &light, const glm::vec3 &cameraPos);

        PointShadowSetup build(const PointLightData &light, uint32_t lightIndex, float nearPlane = 0.05f);

        namespace details
        {
            std::array<glm::mat4, kailux::details::kPointShadowFaceCount> build_faces(
                const glm::vec3 &position,
                float nearPlane,
                float farPlane
            );
        }
    }
}
