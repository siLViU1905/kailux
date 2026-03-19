add_library(kailux_engine STATIC
        src/core/Context.cpp
        src/core/window/Window.cpp
        src/core/Swapchain.cpp
        src/core/Engine.cpp
        src/core/FrameData.cpp
        src/core/CommandRecorder.cpp
        src/core/imgui_backend/ImGuiBackend.cpp
        src/core/imgui_backend/bindings/imgui_impl_glfw.cpp
        src/core/imgui_backend/bindings/imgui_impl_vulkan.cpp
        src/core/window/Event.cpp
)

target_include_directories(kailux_engine PUBLIC src)

target_link_libraries(kailux_engine PUBLIC
        Vulkan::Vulkan
        glfw
        imgui::imgui
)