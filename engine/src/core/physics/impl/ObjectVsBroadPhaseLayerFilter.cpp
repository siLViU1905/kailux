#include "ObjectVsBroadPhaseLayerFilter.h"

namespace kailux::impl
{
    bool ObjectVsBroadPhaseLayerFilter::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
    {
        switch (inLayer1)
        {
            case layers::s_NonMoving:
                return inLayer2 == broad_phase_layers::s_Moving;
            case layers::s_Moving:
                return true;
            default:
                return false;
        }
    }
}
