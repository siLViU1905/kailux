#pragma once
#include "PointShadows.h"
#include "core/Core.h"
#include "core/components/gpu/PointShadowData.h"

namespace kailux
{
    class Scene;

    class PointShadowSet
    {
    public:
        void Update(const Scene &scene, entt::entity camera);

        bool NeedsRedraw(uint32_t slot) const;

        const PointShadowsData &GetData() const;

        const glm::mat4 &GetFace(uint32_t slot, uint32_t face) const;

        bool     Enabled(uint32_t slot) const;
        uint32_t GetActiveCount() const;

        std::array<entt::entity, details::kMaxPointShadows> GetLights() const;

    private:
        struct Slot
        {
            PointShadowSetup setup{};
            entt::entity     light{entt::null};
            uint64_t         signature{};
            bool             dirty{true};
        };

        static uint64_t signature(const Scene& scene, const PointShadowSetup& setup);

        std::array<entt::entity, details::kMaxPointShadows> Select(const Scene &scene, entt::entity camera) const;

        std::array<Slot, details::kMaxPointShadows> mSlots{};
        PointShadowsData                            mData{};
    };
}