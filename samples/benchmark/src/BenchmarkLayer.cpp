#include "BenchmarkLayer.h"
#include <format>

#include "Log.h"

namespace benchmark
{
    BenchmarkLayer::BenchmarkLayer(kailux::Application &application, const std::filesystem::path &outputPath)
        : Layer("BenchmarkLayer"),
          mEngine(application.GetEngine()),
          mOutputPath(outputPath)
    {
    }

    void BenchmarkLayer::OnAttach()
    {
        if (mOutputPath.has_parent_path())
            std::filesystem::create_directories(mOutputPath.parent_path());

        mOutput.open(mOutputPath);
        if (!mOutput)
        {
            kailux::log::console.Error("Could not open the benchmark output '{}'", mOutputPath.generic_string());
            return;
        }

        WriteHeader();
        kailux::log::console.Info("Benchmark readings go to '{}'", mOutputPath.generic_string());
    }

    void BenchmarkLayer::OnDetach()
    {
        if (mOutput.is_open())
            mOutput.close();

        kailux::log::console.Info("Benchmark finished with {} readings in '{}'", mSampleCount, mOutputPath.generic_string());
    }

    void BenchmarkLayer::OnUpdate(float deltaTime)
    {
        mFrameMs = deltaTime * 1000.f;

        if (!IsSimulationRunning())
            return;

        if (mWarmupLeft > 0.f)
        {
            mWarmupLeft -= deltaTime;
            return;
        }

        mRecordingTime += deltaTime;
        mSinceLastSample += deltaTime;

        if (mSinceLastSample < kSampleIntervalSeconds)
            return;

        mSinceLastSample -= kSampleIntervalSeconds;
        WriteSample();
    }

    void BenchmarkLayer::OnImGuiRender(kailux::Scene &scene)
    {
        ImGui::SetNextWindowPos({10.f, 10.f}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.8f);
        if (!ImGui::Begin("Benchmark", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::End();
            return;
        }

        ImGui::Text("Frame: %.3f ms (%.1f FPS)", mFrameMs, mFrameMs > 0.f ? 1000.f / mFrameMs : 0.f);
        if (!IsSimulationRunning())
            ImGui::TextUnformatted("Waiting for the simulation to start");
        else if (mWarmupLeft > 0.f)
            ImGui::Text("Warming up: %.1f s", mWarmupLeft);
        else
            ImGui::Text("Recording: %u readings, one every %.1f s", mSampleCount, kSampleIntervalSeconds);
        ImGui::TextUnformatted(mOutputPath.generic_string().c_str());

        ImGui::SeparatorText("CPU");
        if (ImGui::BeginTable("cpu", 2, ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Section");
            ImGui::TableSetupColumn("Time");
            ImGui::TableHeadersRow();

            const auto& cpu{kailux::profiling::cpu()};
            for (size_t section{}; section < kailux::CpuProfiler::kSectionCount; ++section)
            {
                const auto id{static_cast<kailux::CpuSection>(section)};
                draw_row(kailux::CpuProfiler::get_name(id), cpu.GetTime(id));
            }
            ImGui::EndTable();
        }

        ImGui::SeparatorText("GPU");
        const auto& gpu{kailux::profiling::gpu()};
        if (!gpu.IsSupported())
            ImGui::TextUnformatted("Timestamp queries are not available");

        else if (ImGui::BeginTable("gpu", 3, ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Pass");
            ImGui::TableSetupColumn("Time");
            ImGui::TableSetupColumn("Draws");
            ImGui::TableHeadersRow();

            for (size_t pass{}; pass < kailux::GpuProfiler::kPassCount; ++pass)
            {
                const auto id{static_cast<kailux::GpuSection>(pass)};
                if (const auto time{gpu.GetTime(id)}; time >= 0.f)
                {
                    draw_row(kailux::GpuProfiler::get_name(id), time);
                    ImGui::TableNextColumn();
                    ImGui::Text("%u draws", gpu.GetDraws(id));
                }
            }
            ImGui::EndTable();
        }

        ImGui::End();
    }

    bool BenchmarkLayer::IsSimulationRunning() const
    {
        return mEngine.GetPhysicsSystem().GetSimulationState() == kailux::SimulationState::Running;
    }

    void BenchmarkLayer::WriteHeader()
    {
        mOutput << "# kailux benchmark\n";
        mOutput << std::format("# device: {}\n", mEngine.GetDeviceInfo().deviceName);
        mOutput << std::format("# sample_interval_s: {}\n", kSampleIntervalSeconds);
        mOutput << std::format("# warmup_s: {}\n", kWarmupSeconds);
        mOutput << "# all timings are in milliseconds, an empty gpu value means the pass was not recorded\n";
        mOutput << "# draws_ columns count the draws the GPU executed per pass, every command of an indirect draw counts once\n";

        mOutput << "time_s,frame";
        for (size_t section{}; section < kailux::CpuProfiler::kSectionCount; ++section)
            mOutput << ",cpu_" << kailux::CpuProfiler::get_name(static_cast<kailux::CpuSection>(section));
        for (size_t pass{}; pass < kailux::GpuProfiler::kPassCount; ++pass)
        {
            mOutput << ",gpu_" << kailux::GpuProfiler::get_name(static_cast<kailux::GpuSection>(pass));
            mOutput << ",draws_" << kailux::GpuProfiler::get_name(static_cast<kailux::GpuSection>(pass));
        }
        mOutput << '\n';
        mOutput.flush();
    }

    void BenchmarkLayer::WriteSample()
    {
        if (!mOutput.is_open())
            return;

        mOutput << std::format("{:.3f},{:.4f}", mRecordingTime, mFrameMs);
        for (const auto time : kailux::profiling::cpu().GetTimings())
            mOutput << std::format(",{:.4f}", time);

        const auto& gpu{kailux::profiling::gpu()};
        for (const auto time : gpu.GetTimings())
            if (gpu.IsSupported() && time >= 0.f)
                mOutput << std::format(",{:.4f}", time);
            else
                mOutput << ',';

        for (size_t pass{}; pass < kailux::GpuProfiler::kPassCount; ++pass)
        {
            const auto id{static_cast<kailux::GpuSection>(pass)};
            if (gpu.IsSupported() && gpu.GetTime(id) >= 0.f)
                mOutput << ',' << gpu.GetDraws(id);
            else
                mOutput << ',';
        }
        mOutput << '\n';

        mOutput.flush();
        ++mSampleCount;
    }

    void BenchmarkLayer::draw_row(std::string_view name, float milliseconds)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(name.data(), name.data() + name.size());
        ImGui::TableNextColumn();
        ImGui::Text("%.3f ms", milliseconds);
    }
}
