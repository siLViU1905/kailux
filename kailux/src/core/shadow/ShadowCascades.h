#pragma once
#include "core/Core.h"
#include "core/components/entt/CameraComponent.h"

namespace kailux
{
    struct CascadeBuildInfo
    {
        glm::mat4 cameraView{1.f};
        float     fov{45.f};
        float     aspect{1.f};
        float     zNear{0.1f};
        float     zFar{100.f};

        glm::vec3 lightDirection{0.f, -1.f, 0.f};

        float     splitLambda{0.92f};

        float     depthExtension{50.f};

        float     resolution{static_cast<float>(details::kShadowMapResolution)};
    };

    struct CascadeSetup
    {
        std::array<glm::mat4, details::kShadowCascadeCount> viewProjections{};
        glm::vec4                                           splitDepths{};
    };

    class ShadowCascades
    {
    public:
        static CascadeSetup build(const CascadeBuildInfo &info);

        static CascadeBuildInfo make_build_info(
            const CameraComponent &camera,
            glm::ivec2 extent,
            const glm::vec3 &lightDirection
        );

    private:
        static std::array<float, details::kShadowCascadeCount> compute_splits(
            float zNear,
            float zFar,
            float lambda
        );

        static glm::mat4 build_cascade(
            const CascadeBuildInfo &info,
            const glm::mat4 &inverseView,
            float splitNear,
            float splitFar
        );
    };
}
