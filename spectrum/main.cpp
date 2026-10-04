#include <filesystem>
#include <format>

#include "core/Application.h"
#include "core/Log.h"
#include "core/layers/RuntimeLayer.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        kailux::log::console.Error("Usage: spectrum <scene.klx>");
        return 1;
    }
 
    try
    {
        kailux::log::open_file("spectrum.log");
 
        const std::filesystem::path scenePath{argv[1]};
 
        const kailux::ApplicationSpecification specification{
            {
                1280,
                720,
                std::format("Spectrum - {}", scenePath.stem().string())
            },
            2,
            {
                kailux::RenderMode::Runtime,
                false
            }
        };
 
        kailux::Application application{specification};
        application.PushLayer<kailux::RuntimeLayer>(application, scenePath);
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