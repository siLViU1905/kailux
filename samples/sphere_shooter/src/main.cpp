#include <filesystem>
#include "Application.h"
#include "Log.h"
#include "layers/RuntimeLayer.h"

int main(int argc, char* argv[])
{
    try
    {
        kailux::log::open_file("sphere_shooter.log");

        const std::filesystem::path scenePath{argc > 1 ? argv[1] : "scenes/arena.klx"};

        const kailux::ApplicationSpecification specification{
            {
                1280,
                720,
                "Sphere Shooter"
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