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
            return convert<Ty, timeType>(mDeltaTime);
        }

        template<typename Ty, TimeType timeType>
        Ty getElapsedTime() const
        {
            return convert<Ty, timeType>(std::chrono::steady_clock::now() - mStart);
        }

        static auto now()
        {
            return std::chrono::steady_clock::now();
        }

        template<typename Ty, TimeType timeType>
        static auto get_elapsed(std::chrono::steady_clock::time_point timePoint)
        {
            return convert<Ty, timeType>(now() - timePoint);
        }

    private:
        template<typename Ty, TimeType timeType>
        static Ty convert(std::chrono::duration<float> duration)
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
            else
                static_assert(timeType == TimeType::Microseconds, "Unhandled TimeType");
            return {};
        }

        std::chrono::steady_clock::time_point mStart;
        std::chrono::steady_clock::time_point mLastTick;
        std::chrono::duration<float>          mDeltaTime{0.f};
    };
}
