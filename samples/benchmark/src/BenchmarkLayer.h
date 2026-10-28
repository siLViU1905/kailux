#pragma once
#include <filesystem>
#include <fstream>

#include "Application.h"
#include "layers/Layer.h"
#include "profiling/Profiling.h"

namespace benchmark
{
    class BenchmarkLayer final : public kailux::Layer
    {
    public:
        static constexpr float kSampleIntervalSeconds{0.5f};

        static constexpr float kWarmupSeconds{5.f};

        BenchmarkLayer(kailux::Application& application, const std::filesystem::path& outputPath);

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float deltaTime) override;
        void OnImGuiRender(kailux::Scene& scene) override;

    private:
        bool IsSimulationRunning() const;

        void WriteHeader();
        void WriteSample();

        static void draw_row(std::string_view name, float milliseconds);

        const kailux::Engine& mEngine;

        std::filesystem::path mOutputPath;
        std::ofstream         mOutput;

        float    mFrameMs{};
        float    mWarmupLeft{kWarmupSeconds};
        float    mSinceLastSample{};
        float    mRecordingTime{};
        uint32_t mSampleCount{};
    };
}