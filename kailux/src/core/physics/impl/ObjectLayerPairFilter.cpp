#include "ObjectLayerPairFilter.h"

namespace kailux::impl
{
    bool ObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const
    {
        switch (inLayer1)
        {
            case layers::kNonMoving:
                return inLayer2 == layers::kMoving;
            case layers::kMoving:
                return true;
            default:
                return false;
        }
    }
}
