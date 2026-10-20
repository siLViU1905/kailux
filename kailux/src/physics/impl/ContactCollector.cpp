#include "ContactCollector.h"

namespace kailux::impl
{
    void ContactCollector::OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2,
        const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings)
    {
        std::lock_guard lock{mMutex};
        mRecords.emplace_back(
            inBody1.GetID(),
            inBody2.GetID(),
            inBody1.IsSensor() || inBody2.IsSensor(),
            true
        );
    }

    void ContactCollector::OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair)
    {
        std::lock_guard lock{mMutex};
        mRecords.emplace_back(
            inSubShapePair.GetBody1ID(),
            inSubShapePair.GetBody2ID(),
            false,
            false
        );
    }

    std::vector<ContactRecord> ContactCollector::Drain()
    {
        std::lock_guard lock{mMutex};
        return std::exchange(mRecords, {});
    }
}
