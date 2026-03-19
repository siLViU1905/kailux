#include <print>
#include <core/Window.h>
#include <core/Engine.h>

int main()
{
    try
    {
        auto window = kailux::Window::create(700,400, "Kailux");
        window.updateUserPointer();

        auto engine = kailux::Engine::create(window);

        engine.run(window);
    }
    catch (const std::exception& exception)
    {
        std::println(stderr, "ERROR: {}", exception.what());
    }
}
