#include <print>
#include <iostream>
#include "core/Application.h"

int main()
{
    try
    {
       constexpr kailux::WindowInfo windowInfo(
           700,
           400,
           "Kailux"
           );
        auto application = kailux::Application::create(windowInfo);
        application.run();
    }
    catch (const std::exception& exception)
    {
        std::println(stderr, "Error: {}", exception.what());
        std::println("Press Enter to close...");
        std::cin.get();
        return 1;
    }
}
