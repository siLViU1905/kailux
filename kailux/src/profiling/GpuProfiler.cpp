#include "GpuProfiler.h"

#include <magic_enum/magic_enum.hpp>

#include "Log.h"
#include "buffer/BufferAllocator.h"

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
                                                             mTimings(other.mTimings),
                                                             mReadbackBuffers(std::move(other.mReadbackBuffers)),
                                                             mReadbackSections(other.mReadbackSections),
                                                             mReadbackCounts(other.mReadbackCounts),
                                                             mPendingDraws(other.mPendingDraws),
                                                             mDrawCounts(other.mDrawCounts)
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
            mReadbackBuffers = std::move(other.mReadbackBuffers);
            mReadbackSections = other.mReadbackSections;
            mReadbackCounts = other.mReadbackCounts;
            mPendingDraws = other.mPendingDraws;
            mDrawCounts = other.mDrawCounts;
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

        profiler.CreateReadbackBuffers(context);
        log::console.Debug("gpu profiler: draw count readback buffers created");

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
        mPendingDraws[frameIndex].fill(0);
        mReadbackCounts[frameIndex] = 0;
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

    void GpuProfiler::AddDraws(GpuSection section, uint32_t count)
    {
        if (!IsSupported())
            return;

        mPendingDraws[mCurrentFrame][index(section)] += count;
    }

    void GpuProfiler::AddIndirectDraws(vk::CommandBuffer cmd, vk::Buffer countBuffer, GpuSection section)
    {
        if (!IsSupported())
            return;

        auto& slot{mReadbackCounts[mCurrentFrame]};
        if (slot == kMaxIndirectReadbacks)
            return;

        const auto readback{mReadbackBuffers[mCurrentFrame].GetBuffer()};
        const vk::DeviceSize offset{slot * sizeof(uint32_t)};

        const vk::BufferMemoryBarrier2 beforeCopy{
            vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderWrite,
            vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead,
            vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
            countBuffer,
            0,
            sizeof(uint32_t)
        };
        cmd.pipelineBarrier2(vk::DependencyInfo{{}, {}, beforeCopy, {}});

        cmd.copyBuffer(countBuffer, readback, vk::BufferCopy{0, offset, sizeof(uint32_t)});

        const std::array afterCopy{
            vk::BufferMemoryBarrier2{
                vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                countBuffer,
                0,
                sizeof(uint32_t)
            },
            vk::BufferMemoryBarrier2{
                vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                vk::PipelineStageFlagBits2::eHost, vk::AccessFlagBits2::eHostRead,
                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                readback,
                offset,
                sizeof(uint32_t)
            }
        };
        cmd.pipelineBarrier2(vk::DependencyInfo{{}, {}, afterCopy, {}});

        mReadbackSections[mCurrentFrame][slot] = section;
        ++slot;
    }

    const GpuProfiler::DrawCounts & GpuProfiler::GetDrawCounts() const
    {
        return mDrawCounts;
    }

    uint32_t GpuProfiler::GetDraws(GpuSection section) const
    {
        return mDrawCounts[index(section)];
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

    void GpuProfiler::CreateReadbackBuffers(const Context &context)
    {
        mReadbackBuffers.reserve(details::kFramesInFlight);
        for (uint32_t frame{}; frame < details::kFramesInFlight; ++frame)
            mReadbackBuffers.push_back(buffer_allocator::alloc_host(
                context,
                kMaxIndirectReadbacks * sizeof(uint32_t),
                vk::BufferUsageFlagBits::eTransferDst
            ));
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

        auto draws{mPendingDraws[frameIndex]};
        const auto readback{mReadbackBuffers[frameIndex].Mapped<const uint32_t>()};
        for (uint32_t slot{}; slot < mReadbackCounts[frameIndex]; ++slot)
            draws[index(mReadbackSections[frameIndex][slot])] += readback[slot];

        draws[index(GpuSection::Frame)] = std::accumulate(draws.begin(), draws.end(), 0u);
        mDrawCounts = draws;
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

    uint32_t profiling::imgui_draw_count()
    {
        const auto* drawData{ImGui::GetDrawData()};
        if (!drawData)
            return 0;

        uint32_t count{};
        for (const auto* list : drawData->CmdLists)
            count += static_cast<uint32_t>(list->CmdBuffer.Size);
        return count;
    }
}
