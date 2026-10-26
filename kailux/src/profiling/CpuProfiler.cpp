#include "CpuProfiler.h"

#include <magic_enum/magic_enum.hpp>

namespace kailux
{
    void CpuProfiler::BeginFrame()
    {
        mLast = mCurrent;
        mCurrent.fill(0.f);
    }

    void CpuProfiler::Add(CpuSection section, float milliseconds)
    {
        mCurrent[index(section)] += milliseconds;
    }

    const CpuProfiler::Timings & CpuProfiler::GetTimings() const
    {
        return mLast;
    }

    float CpuProfiler::GetTime(CpuSection section) const
    {
        return mLast[index(section)];
    }

    std::string_view CpuProfiler::get_name(CpuSection section)
    {
        return magic_enum::enum_name(section);
    }

    CpuScope::CpuScope(CpuSection section) : mSection(section),
                                             mStart(std::chrono::steady_clock::now())
    {
    }

    CpuScope::~CpuScope()
    {
        const std::chrono::duration<float, std::milli> elapsed{std::chrono::steady_clock::now() - mStart};
    }

    CpuProfiler & profiling::cpu()
    {
        static CpuProfiler profiler;
        return profiler;
    }
}
