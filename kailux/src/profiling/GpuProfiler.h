#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "../Context.h"
#include "../Core.h"

namespace kailux
{
    enum class GpuSection : uint8_t
    {
        Frame,
        SceneCulling,
        DirectionalShadows,
        PointShadows,
        ScenePass,
        Gizmos,
        Outline,
        Picker,
        SimulationCulling,
        SimulationView,
        Blit,
        ImGui,
        Count
    };

    class GpuProfiler
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(GpuProfiler)

        static constexpr auto kPassCount{static_cast<size_t>(GpuSection::Count)};

        static constexpr uint32_t kMaxScopesPerPass{4};

        using Timings = std::array<float, kPassCount>;

        static constexpr float    kNotRecorded{-1.f};
        static constexpr uint32_t kNoQuery{~0u};

        static GpuProfiler create(const Context& context);

        bool IsSupported() const;

        void BeginFrame(vk::CommandBuffer cmd, uint32_t frameIndex);

        uint32_t Begin(vk::CommandBuffer cmd, GpuSection section);
        void     End(vk::CommandBuffer cmd, uint32_t query);

        const Timings& GetTimings() const;
        float          GetTime(GpuSection section) const;

        static std::string_view get_name(GpuSection section);

    private:
        static constexpr uint32_t kQueryCount{static_cast<uint32_t>(kPassCount) * kMaxScopesPerPass * 2};

        static constexpr size_t index(GpuSection section)
        {
            return static_cast<size_t>(section);
        }

        void CreateQueryPools(const Context &context);
        void Collect(uint32_t frameIndex);

        using ScopeCounts = std::array<uint32_t, kPassCount>;

        std::vector<vk::raii::QueryPool>                       mQueryPools;
        std::array<ScopeCounts, details::kFramesInFlight>      mScopeCounts{};
        uint32_t                                               mCurrentFrame{};

        float    mTimestampPeriod{};
        uint64_t mTimestampMask{};

        Timings mTimings{};
    };

    class GpuScope
    {
    public:
        GpuScope(vk::CommandBuffer cmd, GpuSection section);

        GpuScope(const GpuScope&) = delete;
        GpuScope& operator=(const GpuScope&) = delete;

        ~GpuScope();

    private:
        vk::CommandBuffer mCmd;
        uint32_t          mQuery;
    };

    class GpuFrameScope
    {
    public:
        GpuFrameScope(vk::CommandBuffer cmd, uint32_t frameIndex);

        GpuFrameScope(const GpuFrameScope&) = delete;
        GpuFrameScope& operator=(const GpuFrameScope&) = delete;

        ~GpuFrameScope();

    private:
        vk::CommandBuffer mCmd;
        uint32_t          mQuery;
    };

    namespace profiling
    {
        GpuProfiler& gpu();
    }
}