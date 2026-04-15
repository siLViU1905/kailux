cmake_minimum_required(VERSION 3.31)
project(kailux)

set(CMAKE_CXX_STANDARD 23)

if(EXISTS "${CMAKE_BINARY_DIR}/build/Debug/generators/conan_toolchain.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_BINARY_DIR}/build/Debug/generators/conan_toolchain.cmake" CACHE PATH "")
endif()

find_package(Vulkan 1.4 REQUIRED)
find_package(glfw3 REQUIRED)
find_package(imgui REQUIRED)
find_package(glm REQUIRED)
find_package(EnTT REQUIRED)
find_package(magic_enum REQUIRED)
find_package(imguizmo REQUIRED)
find_package(stb REQUIRED)
find_package(assimp REQUIRED)
find_package(portable-file-dialogs REQUIRED)

get_filename_component(VULKAN_BIN_DIR "${Vulkan_GLSLC_EXECUTABLE}" DIRECTORY)
set(GLSLC_BIN "${VULKAN_BIN_DIR}/glslc.exe")

set(GLSL_SOURCES
        "engine/src/core/shaders/vertex_shader.glsl"
        "engine/src/core/shaders/fragment_shader.glsl"
        "engine/src/core/shaders/skybox_vertex_shader.glsl"
        "engine/src/core/shaders/skybox_fragment_shader.glsl"
)

set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/app/shaders")
file(MAKE_DIRECTORY "${SHADER_BINARY_DIR}")
set(ALL_SPIRV_OUTPUTS "")

foreach(GLSL_SRC ${GLSL_SOURCES})
    get_filename_component(ABS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${GLSL_SRC}" ABSOLUTE)

    if(NOT EXISTS ${ABS_SOURCE})
        message(FATAL_ERROR "GLSL file not found at: ${ABS_SOURCE}")
    endif()

    get_filename_component(FILE_NAME ${GLSL_SRC} NAME_WE)

    if(${GLSL_SRC} MATCHES "vertex")
        set(STAGE "vertex")
    else()
        set(STAGE "fragment")
    endif()

    set(SPV_OUT "${SHADER_BINARY_DIR}/${FILE_NAME}.spv")

    add_custom_command(
            OUTPUT ${SPV_OUT}
            COMMAND ${Vulkan_GLSLC_EXECUTABLE} -fshader-stage=${STAGE} "${ABS_SOURCE}" -o "${SPV_OUT}"
            DEPENDS "${ABS_SOURCE}"
            COMMENT "Compiling SPIR-V: ${FILE_NAME}.spv"
            VERBATIM
    )

    list(APPEND ALL_SPIRV_OUTPUTS ${SPV_OUT})
endforeach()

add_custom_target(CompileShaders ALL DEPENDS ${ALL_SPIRV_OUTPUTS})

set(ASSETS_BINARY_DIR "${CMAKE_BINARY_DIR}/app/assets")

add_custom_target(CopyAssets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${ASSETS_BINARY_DIR}"
        COMMENT "Copying assets folder to ${ASSETS_BINARY_DIR}"
        VERBATIM
)

add_subdirectory(engine)
add_subdirectory(editor)
add_subdirectory(app)

