#pragma once
#include "ShadowCascades.h"
#include "core/components/gpu/DirectionalShadowData.h"

namespace kailux
{
    class Scene;

    class DirectionalShadowSet
    {
    public:
        void Update(const Scene &scene, entt::entity camera, glm::ivec2 extent);

        const DirectionalShadowData &GetData() const;

        const glm::mat4 &GetCascade(uint32_t cascade) const;

        bool Enabled() const;

    private:
        CascadeSetup          mSetup{};
        DirectionalShadowData mData{};
    };

}
