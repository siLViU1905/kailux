#include "ObjectLayerPairFilter.h"

namespace kailux::impl
{
    bool ObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const
    {
        switch (inLayer1)
        {
            case layers::s_NonMoving:
                return inLayer2 == layers::s_Moving;
            case layers::s_Moving:
                return true;
            default:
                return false;
        }
    }
}
