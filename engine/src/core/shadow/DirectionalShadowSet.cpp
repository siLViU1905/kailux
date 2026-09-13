#include "DirectionalShadowSet.h"
#include "core/scene/Scene.h"

namespace kailux
{
    void DirectionalShadowSet::Update(const Scene &scene, entt::entity camera, glm::ivec2 extent)
    {
        mSetup = {};
        mData = {};

        const auto &registry{scene.GetEntityRegistry()};
        const auto sun{scene.GetSun()};

        if (sun == entt::null || !registry.valid(sun) ||
            camera == entt::null || !registry.valid(camera))
            return;

        const auto *sunData{registry.try_get<DirectionalLightData>(sun)};
        if (!sunData || sunData->colorAndEnabled.w < 0.5f)
            return;

        const auto *cameraComponent{registry.try_get<CameraComponent>(camera)};
        if (!cameraComponent)
            return;

        const glm::vec3 direction{sunData->directionAndIntensity};
        if (glm::length(direction) < 0.0001f)
            return;

        mSetup = ShadowCascades::build(
            ShadowCascades::make_build_info(*cameraComponent, extent, direction)
        );

        for (uint32_t i{}; i < details::kShadowCascadeCount; ++i)
            mData.cascades[i].viewProjection = mSetup.viewProjections[i];

        mData.splitDepths = mSetup.splitDepths;
        mData.params.x = 1.f;
    }

    const DirectionalShadowData &DirectionalShadowSet::GetData() const
    {
        return mData;
    }

    const glm::mat4 &DirectionalShadowSet::GetCascade(uint32_t cascade) const
    {
        assert(cascade < mSetup.viewProjections.size() && "Cascade index out of range");
        return mSetup.viewProjections[cascade];
    }

    bool DirectionalShadowSet::Enabled() const
    {
        return mData.params.x > 0.5f;
    }
}