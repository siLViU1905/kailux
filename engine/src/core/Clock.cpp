#include "Clock.h"

namespace kailux
{
    Clock::Clock()
    {
        mStart = std::chrono::steady_clock::now();
        mLastTick = mStart;
    }

    void Clock::tick()
    {
        auto now = std::chrono::steady_clock::now();
        mDeltaTime = now - mLastTick;
        mLastTick  = now;
    }
}