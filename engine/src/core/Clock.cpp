#include "Clock.h"

namespace kailux
{
    Clock::Clock()
    {
        m_Start = std::chrono::steady_clock::now();
        m_LastTick = m_Start;
    }

    void Clock::tick()
    {
        auto now = std::chrono::steady_clock::now();
        m_DeltaTime = now - m_LastTick;
        m_LastTick  = now;
    }
}