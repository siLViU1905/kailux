#include "ObjectVsBroadPhaseLayerFilter.h"

namespace kailux::impl
{
    bool ObjectVsBroadPhaseLayerFilter::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
    {
        switch (inLayer1)
        {
            case layers::kNonMoving:
                return inLayer2 == broad_phase_layers::kMoving;
            case layers::kMoving:
                return true;
            default:
                return false;
        }
    }
}
