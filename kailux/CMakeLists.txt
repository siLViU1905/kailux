find_file(IMGUIZMO_CPP ImGuizmo.cpp PATHS ${imguizmo_INCLUDE_DIRS}/../src)

add_library(kailux STATIC
        src/Context.cpp
        src/window/Window.cpp
        src/Swapchain.cpp
        src/Engine.cpp
        src/FrameData.cpp
        src/command/CommandRecorder.cpp
        src/imgui_backend/ImGuiBackend.cpp
        src/imgui_backend/bindings/imgui_impl_glfw.cpp
        src/imgui_backend/bindings/imgui_impl_vulkan.cpp
        src/imgui_backend/misc/imgui_stdlib.cpp
        ${IMGUIZMO_CPP}
        src/window/Event.cpp
        src/buffer/Buffer.cpp
        src/buffer/BufferAllocator.cpp
        src/mesh/MeshRegistry.cpp
        src/descriptor/DescriptorLayout.cpp
        src/command/OneTimeCommand.cpp
        src/Camera.cpp
        src/descriptor/DescriptorPool.cpp
        src/descriptor/DescriptorSet.cpp
        src/Clock.cpp
        src/scene/Scene.cpp
        src/texture/Texture.cpp
        src/texture/TextureAllocator.cpp
        src/texture/ImageLoader.cpp
        src/texture/TextureRegistry.cpp
        src/texture/TextureMemoryPool.cpp
        src/mesh/MeshLoader.cpp
        src/utilities/ThreadDispatcher.cpp
        src/utilities/LinearZone.cpp
        src/utilities/FreeListZone.cpp
        src/utilities/Bounds.cpp
        src/passes/SkyboxPass.cpp
        src/passes/ComputePicker.cpp
        src/passes/OutlinePass.cpp
        src/passes/GraphicsPass.cpp
        src/passes/MainPass.cpp
        src/passes/ComputePass.cpp
        src/passes/ComputeCuller.cpp
        src/passes/ShadowPass.cpp
        src/Geometry.cpp
        src/physics/impl/BroadPhaseLayer.cpp
        src/physics/impl/ObjectVsBroadPhaseLayerFilter.cpp
        src/physics/impl/ObjectLayerPairFilter.cpp
        src/physics/impl/ContactCollector.cpp
        src/physics/PhysicsRegistry.cpp
        src/TransferManager.cpp
        src/AssetPipeline.cpp
        src/physics/PhysicsSystem.cpp
        src/passes/GizmoPass.cpp
        src/gizmo/GizmoGeometry.cpp
        src/gizmo/GizmoRegistry.cpp
        src/mesh/MeshGeometry.cpp
        src/Shader.cpp
        src/scene/SceneSerializer.cpp
        src/scene/SceneInstantiator.cpp
        src/scene/AssetPaths.cpp
        src/SimulationView.cpp
        src/window/InputSource.cpp
        src/pipeline/GraphicsPipeline.cpp
        src/pipeline/ComputePipeline.cpp
        src/components/math/Transform.cpp
        src/shadow/ShadowMap.cpp
        src/shadow/ShadowCascades.cpp
        src/shadow/PointShadows.cpp
        src/shadow/DirectionalShadowSet.cpp
        src/shadow/PointShadowSet.cpp
        src/shadow/ShadowCuller.cpp
        src/RenderTarget.cpp
        src/scripting/ScriptSystem.cpp
        src/scripting/ScriptContext.cpp
        src/scripting/Script.cpp
        src/scripting/Entity.cpp
        src/scripting/Input.cpp
        src/scripting/World.cpp
        src/scripting/Time.cpp
        src/scripting/ScriptRegistry.cpp
        src/scripting/CameraRef.cpp
        src/scripting/AudioSource.cpp
        src/layers/Layer.cpp
        src/layers/LayerStack.cpp
        src/Application.cpp
        src/layers/RuntimeLayer.cpp
        src/scene/SceneManager.cpp
        src/audio/AudioLoader.cpp
        src/audio/AudioRegistry.cpp
        src/audio/AudioSystem.cpp
)

target_compile_definitions(kailux PUBLIC
        IMGUI_DEFINE_MATH_OPERATORS
)

if(KAILUX_ENABLE_PROFILING)
    target_sources(kailux PRIVATE
            src/profiling/CpuProfiler.cpp
            src/profiling/GpuProfiler.cpp
    )
    target_compile_definitions(kailux PUBLIC KAILUX_ENABLE_PROFILING)
    message(STATUS "Kailux profiling: enabled")
endif()

if(MSVC)
    set(SIMD_FLAGS "/arch:AVX2")
else()
    set(SIMD_FLAGS -mavx2 -mfma -msse3)
endif()

target_compile_options(kailux PUBLIC ${SIMD_FLAGS})

target_include_directories(kailux PUBLIC src)

target_link_libraries(kailux PUBLIC
        Vulkan::Vulkan
        glfw
        imgui::imgui
        glm::glm
        EnTT::EnTT
        magic_enum::magic_enum
        imguizmo::imguizmo
        stb::stb
        assimp::assimp
        portable-file-dialogs::portable-file-dialogs
        nlohmann_json::nlohmann_json
        Jolt::Jolt
        shaderc::shaderc
        TBB::tbb
        meshoptimizer::meshoptimizer
        OpenAL::OpenAL
        dr_libs::dr_libs
)

target_precompile_headers(kailux PUBLIC
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
        <nlohmann/json.hpp>
)