#pragma once

#include "Layers.h"

namespace kailux::impl
{
    class ObjectLayerPairFilter : public JPH::ObjectLayerPairFilter
    {
    public:
        bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override;
    };
}
