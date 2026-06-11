#include "BroadPhaseLayer.h"

namespace kailux::impl
{
    BroadPhaseLayer::BroadPhaseLayer()
    {
        m_ObjectToBroadPhase[layers::s_NonMoving] = broad_phase_layers::s_NonMoving;
        m_ObjectToBroadPhase[layers::s_Moving] = broad_phase_layers::s_Moving;
    }

    JPH::uint BroadPhaseLayer::GetNumBroadPhaseLayers() const
    {
        return broad_phase_layers::s_LayersCount;
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
            case broad_phase_layers::s_NonMoving:
                return "NON_MOVING";
            case broad_phase_layers::s_Moving:
                return "MOVING";
            default:
                return "UNKNOWN";
        }
    }
#endif
}
