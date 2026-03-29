add_library(kailux_engine STATIC
        src/core/Context.cpp
        src/core/window/Window.cpp
        src/core/Swapchain.cpp
        src/core/Engine.cpp
        src/core/FrameData.cpp
        src/core/command/CommandRecorder.cpp
        src/core/imgui_backend/ImGuiBackend.cpp
        src/core/imgui_backend/bindings/imgui_impl_glfw.cpp
        src/core/imgui_backend/bindings/imgui_impl_vulkan.cpp
        src/core/window/Event.cpp
        src/core/buffer/Buffer.cpp
        src/core/buffer/BufferAllocator.cpp
        src/core/mesh/MeshRegistry.cpp
        src/core/Pipeline.cpp
        src/core/Pipeline.h
        src/core/descriptor/DescriptorLayout.cpp
        src/core/command/OneTimeCommand.cpp
        src/core/Camera.cpp
        src/core/descriptor/DescriptorPool.cpp
        src/core/descriptor/DescriptorSet.cpp
        src/core/Clock.cpp
        src/core/Scene.cpp
)

target_include_directories(kailux_engine PUBLIC src)

target_link_libraries(kailux_engine PUBLIC
        Vulkan::Vulkan
        glfw
        imgui::imgui
        glm::glm
        EnTT::EnTT
        magic_enum::magic_enum
)

target_precompile_headers(kailux_engine PUBLIC
        <vulkan/vulkan.hpp>
        <vulkan/vulkan_raii.hpp>
        <glm/glm.hpp>
        <glm/gtc/matrix_transform.hpp>
        <entt/entt.hpp>
        <imgui.h>
        <vector>
        <array>
        <memory>
        <string>
)