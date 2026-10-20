#pragma once

#include <mutex>
#include <vector>
#include "Layers.h"
#include <Jolt/Physics/Collision/ContactListener.h>

namespace kailux::impl
{
    struct ContactRecord
    {
        JPH::BodyID first;
        JPH::BodyID second;
        bool        sensor{};
        bool        began{};
    };

    class ContactCollector final : public JPH::ContactListener
    {
    public:
        void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override;
        void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override;

        std::vector<ContactRecord> Drain();

    private:
        std::mutex                 mMutex;
        std::vector<ContactRecord> mRecords;
    };
}