#include <print>
#include <iostream>
#include <core/window/Window.h>
#include <core/Engine.h>

#include "Editor.h"

int main()
{
    try
    {
        auto window = kailux::Window::create(700,400, "Kailux");
        window.updateUserPointer();

        auto engine = kailux::Engine::create(window);

        auto editor = kailux::Editor::create();
        engine.setOnEditorRender([&editor](kailux::Scene& scene)
        {
            editor.render(scene);
        });

        engine.run(window);
    }
    catch (const std::exception& exception)
    {
        std::println(stderr, "Error: {}", exception.what());
        std::println("Press Enter to close...");
        std::cin.get();
        return 1;
    }
}
