#pragma once
#include <chrono>

namespace kailux
{
    enum class TimeType
    {
        Microseconds,
        Milliseconds,
        Seconds
    };

    class Clock
    {
    public:
        Clock();

        void tick();

        template<typename Ty, TimeType timeType>
        Ty getDeltaTime() const
        {
            return convert<Ty, timeType>(m_DeltaTime);
        }

        template<typename Ty, TimeType timeType>
        Ty getElapsedTime() const
        {
            return convert<Ty, timeType>(std::chrono::steady_clock::now() - m_Start);
        }

    private:
        template<typename Ty, TimeType timeType>
        static Ty convert(std::chrono::duration<double> duration)
        {
            if constexpr (timeType == TimeType::Milliseconds)
                return static_cast<Ty>(
                    std::chrono::duration_cast<std::chrono::duration<Ty, std::milli> >(duration).count()
                );
            else if constexpr (timeType == TimeType::Seconds)
                return static_cast<Ty>(
                    std::chrono::duration_cast<std::chrono::duration<Ty> >(duration).count()
                );
            else if constexpr (timeType == TimeType::Microseconds)
                return static_cast<Ty>(
                    std::chrono::duration_cast<std::chrono::duration<Ty, std::micro> >(duration).count()
                );
            return {};
        }

        std::chrono::steady_clock::time_point m_Start;
        std::chrono::steady_clock::time_point m_LastTick;
        std::chrono::duration<float>          m_DeltaTime;
    };
}
