#include "GpuProfiler.h"

#include <magic_enum/magic_enum.hpp>

#include "Log.h"

namespace kailux
{
    GpuProfiler::GpuProfiler()
    {
        mTimings.fill(kNotRecorded);
    }

    GpuProfiler::GpuProfiler(GpuProfiler &&other) noexcept : mQueryPools(std::move(other.mQueryPools)),
                                                             mScopeCounts(other.mScopeCounts),
                                                             mCurrentFrame(other.mCurrentFrame),
                                                             mTimestampPeriod(other.mTimestampPeriod),
                                                             mTimestampMask(other.mTimestampMask),
                                                             mTimings(other.mTimings)
    {
    }

    GpuProfiler &GpuProfiler::operator=(GpuProfiler &&other) noexcept
    {
        if (this != &other)
        {
            mQueryPools = std::move(other.mQueryPools);
            mScopeCounts = other.mScopeCounts;
            mCurrentFrame = other.mCurrentFrame;
            mTimestampPeriod = other.mTimestampPeriod;
            mTimestampMask = other.mTimestampMask;
            mTimings = other.mTimings;
        }
        return *this;
    }

    GpuProfiler GpuProfiler::create(const Context &context)
    {
        GpuProfiler profiler;

        const auto physicalDevice{context.GetPhysicalDevice()};
        const auto families{physicalDevice.getQueueFamilyProperties()};
        const auto validBits{families[context.GetGraphicsQueueFamilyIndex()].timestampValidBits};
        if (validBits == 0)
        {
            log::console.Warning("gpu profiler: the graphics queue does not support timestamps, gpu timings are disabled");
            return profiler;
        }

        profiler.mTimestampPeriod = physicalDevice.getProperties().limits.timestampPeriod;
        profiler.mTimestampMask = validBits >= 64 ? ~0ull : (1ull << validBits) - 1;

        profiler.CreateQueryPools(context);
        log::console.Debug("gpu profiler: query pools created");

        return profiler;
    }

    bool GpuProfiler::IsSupported() const
    {
        return !mQueryPools.empty();
    }

    void GpuProfiler::BeginFrame(vk::CommandBuffer cmd, uint32_t frameIndex)
    {
        if (!IsSupported())
            return;

        Collect(frameIndex);

        mCurrentFrame = frameIndex;
        mScopeCounts[frameIndex].fill(0);
        cmd.resetQueryPool(*mQueryPools[frameIndex], 0, kQueryCount);
    }

    uint32_t GpuProfiler::Begin(vk::CommandBuffer cmd, GpuSection section)
    {
        if (!IsSupported())
            return kNoQuery;

        auto& count{mScopeCounts[mCurrentFrame][index(section)]};
        if (count == kMaxScopesPerPass)
            return kNoQuery;

        const auto query{(static_cast<uint32_t>(section) * kMaxScopesPerPass + count) * 2};
        ++count;

        cmd.writeTimestamp2(vk::PipelineStageFlagBits2::eAllCommands, *mQueryPools[mCurrentFrame], query);
        return query;
    }

    void GpuProfiler::End(vk::CommandBuffer cmd, uint32_t query)
    {
        if (query == kNoQuery)
            return;

        cmd.writeTimestamp2(vk::PipelineStageFlagBits2::eAllCommands, *mQueryPools[mCurrentFrame], query + 1);
    }

    const GpuProfiler::Timings & GpuProfiler::GetTimings() const
    {
        return mTimings;
    }

    float GpuProfiler::GetTime(GpuSection section) const
    {
        return mTimings[index(section)];
    }

    std::string_view GpuProfiler::get_name(GpuSection section)
    {
        return magic_enum::enum_name(section);
    }

    void GpuProfiler::CreateQueryPools(const Context &context)
    {
        const vk::QueryPoolCreateInfo createInfo{
            {},
            vk::QueryType::eTimestamp,
            kQueryCount
        };

        mQueryPools.reserve(details::kFramesInFlight);
        for (uint32_t frame{}; frame < details::kFramesInFlight; ++frame)
            mQueryPools.emplace_back(context.GetRAIIDevice(), createInfo);
    }

    void GpuProfiler::Collect(uint32_t frameIndex)
    {
        const auto& counts{mScopeCounts[frameIndex]};
        if (counts[index(GpuSection::Frame)] == 0)
            return;

        const auto [result, ticks]{
            mQueryPools[frameIndex].getResults<uint64_t>(
                0,
                kQueryCount,
                kQueryCount * sizeof(uint64_t),
                sizeof(uint64_t),
                vk::QueryResultFlagBits::e64
            )
        };

        for (size_t pass{}; pass < kPassCount; ++pass)
        {
            if (counts[pass] == 0)
            {
                mTimings[pass] = kNotRecorded;
                continue;
            }

            uint64_t elapsedTicks{};
            for (uint32_t scope{}; scope < counts[pass]; ++scope)
            {
                const auto query{(pass * kMaxScopesPerPass + scope) * 2};
                elapsedTicks += (ticks[query + 1] - ticks[query]) & mTimestampMask;
            }
            mTimings[pass] = static_cast<float>(static_cast<double>(elapsedTicks) * mTimestampPeriod / 1'000'000.0);
        }
    }

    GpuScope::GpuScope(vk::CommandBuffer cmd, GpuSection section) : mCmd(cmd),
                                                                    mQuery(profiling::gpu().Begin(cmd, section))
    {
    }

    GpuScope::~GpuScope()
    {
        profiling::gpu().End(mCmd, mQuery);
    }

    GpuFrameScope::GpuFrameScope(vk::CommandBuffer cmd, uint32_t frameIndex) : mCmd(cmd)
    {
        profiling::gpu().BeginFrame(cmd, frameIndex);
        mQuery = profiling::gpu().Begin(cmd, GpuSection::Frame);
    }

    GpuFrameScope::~GpuFrameScope()
    {
        profiling::gpu().End(mCmd, mQuery);
    }

    GpuProfiler & profiling::gpu()
    {
        static GpuProfiler profiler;
        return profiler;
    }
}
