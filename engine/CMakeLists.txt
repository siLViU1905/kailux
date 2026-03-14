add_library(kailux_engine STATIC
        src/core/Context.cpp
        src/core/Window.cpp
)

target_include_directories(kailux_engine PUBLIC src)

target_link_libraries(kailux_engine PUBLIC
        Vulkan::Vulkan
        glfw
)