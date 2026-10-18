#pragma once

namespace kailux
{
    class Time
    {
    public:
        float Delta() const;
        float Elapsed() const;
        float FixedStep() const;

    private:
        void Advance(float deltaTime);
        void Reset();

        float mDelta{};
        float mElapsed{};
        float mFixedStep{1.f / 60.f};

        friend class ScriptSystem;
    };
}