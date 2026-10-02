#pragma once
#include "../Core.h"
#include "core/components/gpu/PointLightData.h"

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

    class PointShadows
    {
    public:
        static constexpr float kStickiness{1.25f};

        static float score(const PointLightData& light, const glm::vec3& cameraPos);

        static PointShadowSetup build(const PointLightData &light, uint32_t lightIndex, float nearPlane = 0.05f);

    private:
        static std::array<glm::mat4, details::kPointShadowFaceCount> build_faces(
            const glm::vec3 &position,
            float nearPlane,
            float farPlane
        );
    };
}
