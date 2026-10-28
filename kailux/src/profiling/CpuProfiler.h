#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <string_view>

namespace kailux
{
    enum class CpuSection : uint8_t
    {
        AssetPipeline,
        TransferManager,
        ResourceEraser,
        PhysicsSystem,
        ScriptSystem,
        SceneRegistry,
        CameraControl,
        AudioSystem,
        EngineUpdate,
        FrameWait,
        RenderRecord,
        SubmitPresent,
        Count
    };

    class CpuProfiler
    {
    public:
        static constexpr auto kSectionCount{static_cast<size_t>(CpuSection::Count)};

        using Timings = std::array<float, kSectionCount>;

        void BeginFrame();

        void Add(CpuSection section, float milliseconds);

        const Timings& GetTimings() const;
        float          GetTime(CpuSection section) const;

        static std::string_view get_name(CpuSection section);

    private:
        using TimePoint = std::chrono::steady_clock::time_point;

        static constexpr size_t index(CpuSection section)
        {
            return static_cast<size_t>(section);
        }

        Timings                              mCurrent{};
        Timings                              mLast{};
    };

    class CpuScope
    {
    public:
        explicit CpuScope(CpuSection section);

        CpuScope(const CpuScope&) = delete;
        CpuScope& operator=(const CpuScope&) = delete;

        ~CpuScope();

    private:
        CpuSection                            mSection;
        std::chrono::steady_clock::time_point mStart;
    };

    namespace profiling
    {
        CpuProfiler& cpu();
    }
}