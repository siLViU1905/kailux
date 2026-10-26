#pragma once

#ifdef KAILUX_PROFILING

#include "CpuProfiler.h"
#include "GpuProfiler.h"

#define KAILUX_PROFILE_CONCAT_IMPL(a, b) a##b
#define KAILUX_PROFILE_CONCAT(a, b) KAILUX_PROFILE_CONCAT_IMPL(a, b)


#define KAILUX_PROFILE_CPU(section) \
    const kailux::CpuScope KAILUX_PROFILE_CONCAT(kailuxCpuScope, __LINE__){section}

#define KAILUX_PROFILE_CPU_FRAME() kailux::profiling::cpu().BeginFrame()

#define KAILUX_PROFILE_GPU(cmd, section) \
    const kailux::GpuScope KAILUX_PROFILE_CONCAT(kailuxGpuScope, __LINE__){cmd, section}

#define KAILUX_PROFILE_GPU_FRAME(cmd, frameIndex) \
    const kailux::GpuFrameScope KAILUX_PROFILE_CONCAT(kailuxGpuFrameScope, __LINE__){cmd, frameIndex}


#define KAILUX_PROFILE_GPU_INIT(context) kailux::profiling::gpu() = kailux::GpuProfiler::create(context)
#define KAILUX_PROFILE_GPU_SHUTDOWN() kailux::profiling::gpu() = kailux::GpuProfiler{}

#else

#define KAILUX_PROFILE_CPU(section)
#define KAILUX_PROFILE_CPU_FRAME()
#define KAILUX_PROFILE_GPU(cmd, pass)
#define KAILUX_PROFILE_GPU_FRAME(cmd, frameIndex)
#define KAILUX_PROFILE_GPU_INIT(context)
#define KAILUX_PROFILE_GPU_SHUTDOWN()

#endif