#include <chrono>
#include <filesystem>
#include <format>

#include "Application.h"
#include "BenchmarkLayer.h"
#include "Log.h"
#include "layers/RuntimeLayer.h"

namespace
{
    std::filesystem::path default_output_path()
    {
        const auto now{std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now())};
        return std::filesystem::path{"benchmarks"} / std::format("benchmark_{:%Y%m%d_%H%M%S}.txt", now);
    }
}

int main(int argc, char* argv[])
{
    try
    {
        kailux::log::open_file("sphere_shooter_benchmark.log");

        const std::filesystem::path scenePath{argc > 1 ? argv[1] : "scenes/arena-9500.klx"};
        const std::filesystem::path outputPath{argc > 2 ? argv[2] : default_output_path()};

        kailux::EngineSpecification engine;
        engine.renderMode = kailux::RenderMode::Runtime;
        engine.useImgui = true;

        const kailux::ApplicationSpecification specification{
            {
                1280,
                720,
                "Sphere Shooter Benchmark"
            },
            2,
            engine
        };

        kailux::Application application{specification};
        application.PushLayer<kailux::RuntimeLayer>(application, scenePath);
        application.PushOverlay<benchmark::BenchmarkLayer>(application, outputPath);
        application.Run();

        kailux::log::close_file();
    }
    catch (const std::exception& exception)
    {
        kailux::log::file.Error("{}", exception.what());
        kailux::log::console.Error("{}", exception.what());
        kailux::log::close_file();
        return 1;
    }
}