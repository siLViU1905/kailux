#include "Time.h"

namespace kailux
{
    float Time::Delta() const
    {
        return mDelta;
    }

    float Time::Elapsed() const
    {
        return mElapsed;
    }

    float Time::FixedStep() const
    {
        return mFixedStep;
    }

    void Time::Advance(float deltaTime)
    {
        mDelta = deltaTime;
        mElapsed += deltaTime;
    }

    void Time::Reset()
    {
        mDelta = 0.f;
        mElapsed = 0.f;
    }
}
