#include <print>
#include <core/Window.h>
#include <core/Context.h>

int main()
{
    try
    {
        auto window = kailux::Window::create(700,400, "Test");
        window.updateUserPointer();
        auto context = kailux::Context::create(window);
    }
    catch (const std::exception& exception)
    {
        std::println(stderr, "ERROR: {}", exception.what());
    }
}
