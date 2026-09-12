#include "ShadowCascades.h"

namespace kailux
{
    CascadeSetup ShadowCascades::build(const CascadeBuildInfo &info)
    {
        CascadeSetup setup;

        const auto splits{compute_splits(info.zNear, info.zFar, info.splitLambda)};
        const auto inverseView{glm::inverse(info.cameraView)};

        float splitNear{info.zNear};
        for (uint32_t i{}; i < details::kShadowCascadeCount; ++i)
        {
            const float splitFar{splits[i]};

            setup.viewProjections[i] = build_cascade(info, inverseView, splitNear, splitFar);
            setup.splitDepths[static_cast<int>(i)] = splitFar;

            splitNear = splitFar;
        }

        return setup;
    }

    CascadeBuildInfo ShadowCascades::make_build_info(
        const CameraComponent &camera,
        glm::ivec2 extent,
        const glm::vec3 &lightDirection)
    {
        CascadeBuildInfo info;
        info.cameraView = glm::lookAt(camera.position, camera.position + camera.forward, camera.up);
        info.fov = camera.fov;
        info.aspect = extent.y > 0
                          ? static_cast<float>(extent.x) / static_cast<float>(extent.y)
                          : 1.f;
        info.zNear = camera.zNear;
        info.zFar = camera.zFar;
        info.lightDirection = lightDirection;
        return info;
    }

    std::array<float, details::kShadowCascadeCount> ShadowCascades::compute_splits(
        float zNear,
        float zFar,
        float lambda)
    {
        std::array<float, details::kShadowCascadeCount> splits{};

        const float range{zFar - zNear};
        const float ratio{zFar / std::max(zNear, 0.0001f)};

        for (uint32_t i{}; i < details::kShadowCascadeCount; ++i)
        {
            const float p{static_cast<float>(i + 1) / static_cast<float>(details::kShadowCascadeCount)};

            const float logarithmic{zNear * std::pow(ratio, p)};
            const float uniform{zNear + range * p};

            splits[i] = lambda * (logarithmic - uniform) + uniform;
        }

        splits[details::kShadowCascadeCount - 1] = zFar;

        return splits;
    }

    glm::mat4 ShadowCascades::build_cascade(
        const CascadeBuildInfo &info,
        const glm::mat4 &inverseView,
        float splitNear,
        float splitFar)
    {
        const float tanHalfFov{std::tan(glm::radians(info.fov) * 0.5f)};

        const float nearHeight{tanHalfFov * splitNear};
        const float nearWidth{nearHeight * info.aspect};
        const float farHeight{tanHalfFov * splitFar};
        const float farWidth{farHeight * info.aspect};

        const std::array viewCorners{
            glm::vec3(-nearWidth, -nearHeight, -splitNear),
            glm::vec3(nearWidth, -nearHeight, -splitNear),
            glm::vec3(nearWidth, nearHeight, -splitNear),
            glm::vec3(-nearWidth, nearHeight, -splitNear),
            glm::vec3(-farWidth, -farHeight, -splitFar),
            glm::vec3(farWidth, -farHeight, -splitFar),
            glm::vec3(farWidth, farHeight, -splitFar),
            glm::vec3(-farWidth, farHeight, -splitFar)
        };

        std::array<glm::vec3, 8> worldCorners{};
        glm::vec3 center{};
        for (size_t i{}; i < viewCorners.size(); ++i)
        {
            worldCorners[i] = glm::vec3(inverseView * glm::vec4(viewCorners[i], 1.f));
            center += worldCorners[i];
        }
        center /= static_cast<float>(worldCorners.size());

        float radius{};
        for (const auto &corner: worldCorners)
            radius = std::max(radius, glm::length(corner - center));

        radius = std::ceil(radius * 16.f) / 16.f;

        const glm::vec3 direction{glm::normalize(info.lightDirection)};
        const glm::vec3 up{std::abs(direction.y) > 0.99f
                                 ? glm::vec3(0.f, 0.f, 1.f)
                                 : glm::vec3(0.f, 1.f, 0.f)};

        const glm::vec3 eye{center - direction * (radius + info.depthExtension)};
        const glm::mat4 lightView{glm::lookAt(eye, center, up)};

        auto lightProjection{glm::orthoRH_ZO(
            -radius,
            radius,
            -radius,
            radius,
            0.f,
            2.f * radius + info.depthExtension
        )};

        const auto viewProjection{lightProjection * lightView};

        const auto shadowOrigin{viewProjection * glm::vec4(0.f, 0.f, 0.f, 1.f)};
        const float halfResolution{info.resolution * 0.5f};

        const auto texelOrigin{glm::vec2(shadowOrigin) * halfResolution};
        const auto rounded{glm::round(texelOrigin)};
        const auto offset{(rounded - texelOrigin) / halfResolution};

        lightProjection[3][0] += offset.x;
        lightProjection[3][1] += offset.y;

        return lightProjection * lightView;
    }
}
