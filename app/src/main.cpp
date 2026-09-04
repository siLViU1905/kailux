#include <print>
#include <iostream>
#include "core/Application.h"
#include "core/Log.h"

int main()
{
    try
    {
        kailux::log::open_file("kailux.log");

        constexpr kailux::WindowInfo windowInfo{
            700,
            400,
            "Kailux"
        };

        kailux::Application application{windowInfo};
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
