#pragma once

#include "Layers.h"

namespace kailux::impl
{
    class BroadPhaseLayer final : public JPH::BroadPhaseLayerInterface
    {
    public:
        BroadPhaseLayer();

        JPH::uint            GetNumBroadPhaseLayers() const override;
        JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif
    private:
        std::array<JPH::BroadPhaseLayer, layers::kLayersCount> mObjectToBroadPhase;
    };
}
