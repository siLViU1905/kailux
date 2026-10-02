#include "BroadPhaseLayer.h"

namespace kailux::impl
{
    BroadPhaseLayer::BroadPhaseLayer()
    {
        mObjectToBroadPhase[layers::kNonMoving] = broad_phase_layers::kNonMoving;
        mObjectToBroadPhase[layers::kMoving] = broad_phase_layers::kMoving;
    }

    JPH::uint BroadPhaseLayer::GetNumBroadPhaseLayers() const
    {
        return broad_phase_layers::kLayersCount;
    }

    JPH::BroadPhaseLayer BroadPhaseLayer::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
    {
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
    {
        switch (inLayer)
        {
            case broad_phase_layers::kNonMoving:
                return "NON_MOVING";
            case broad_phase_layers::kMoving:
                return "MOVING";
            default:
                return "UNKNOWN";
        }
    }
#endif
}
