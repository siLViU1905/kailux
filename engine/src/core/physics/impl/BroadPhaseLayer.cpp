#include "BroadPhaseLayer.h"

namespace kailux::impl
{
    BroadPhaseLayer::BroadPhaseLayer()
    {
        m_ObjectToBroadPhase[layers::kNonMoving] = broad_phase_layers::kNonMoving;
        m_ObjectToBroadPhase[layers::kMoving] = broad_phase_layers::kMoving;
    }

    JPH::uint BroadPhaseLayer::GetNumBroadPhaseLayers() const
    {
        return broad_phase_layers::kLayersCount;
    }

    JPH::BroadPhaseLayer BroadPhaseLayer::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
    {
        return m_ObjectToBroadPhase[inLayer];
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
