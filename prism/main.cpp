#include "core/Application.h"
#include "core/Log.h"
#include "core/layers/EditorLayer.h"

int main()
{
    try
    {
        kailux::log::open_file("kailux.log");

        const kailux::ApplicationSpecification specification{
            {
                1280,
                720,
                "Kailux"
            },
            2,
            {
                kailux::RenderMode::Editor
            }
        };

        kailux::Application application{specification};
        application.PushLayer<kailux::EditorLayer>(application);
        application.Run();

        kailux::log::close_file();
    }
    catch (const std::exception& exception)
    {
        kailux::log::file.Error("{}", exception.what());
        kailux::log::close_file();
        return 1;
    }
}

