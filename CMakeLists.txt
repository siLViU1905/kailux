cmake_minimum_required(VERSION 3.31)
project(kailux)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_CXX_STANDARD 23)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "ccache enabled: ${CCACHE_PROGRAM}")
endif()

if(WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(NOT MSVC)
    find_program(MOLD_LINKER mold)
    find_program(LLD_LINKER ld.lld)
    if(MOLD_LINKER)
        add_link_options(-fuse-ld=mold)
        message(STATUS "Linker: mold (${MOLD_LINKER})")
    elseif(LLD_LINKER)
        add_link_options(-fuse-ld=lld)
        message(STATUS "Linker: lld (${LLD_LINKER})")
    else()
        message(STATUS "Linker: default (install mold or lld for faster links)")
    endif()
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

if(ipo_supported AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    message(STATUS "LTO activated for Release")
elseif(NOT ipo_supported)
    message(WARNING "LTO not supported: ${ipo_error}")
endif()

if(MSVC)
    add_compile_options(/W3 /permissive-)
else()
    add_compile_options(
            -Wall
            -Wno-unused-parameter
            -Wno-unused-variable
            -Wno-missing-field-initializers
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        add_compile_options(/RTC1)
    else()
        add_compile_options(-fno-omit-frame-pointer -g3)
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/O2 /Oi /Ot /fp:fast)
    else()
        add_compile_options(-O2 -ffast-math)
    endif()
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
find_package(nlohmann_json REQUIRED)
find_package(Jolt REQUIRED)
find_package(shaderc REQUIRED)
find_package(TBB REQUIRED)
find_package(meshoptimizer REQUIRED)
find_package(OpenAL REQUIRED)
find_package(dr_libs REQUIRED)

set(KAILUX_SHADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/kailux/src/shaders")
set(KAILUX_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(KAILUX_LICENSES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/licenses")

get_filename_component(KAILUX_CONAN_GENERATORS_DIR "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
set(KAILUX_RUNTIME_LIBS_DIR "${KAILUX_CONAN_GENERATORS_DIR}/runtime")

function(kailux_copy_resources target)
    add_custom_target(${target}_resources
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${KAILUX_SHADERS_DIR}"
            "$<TARGET_FILE_DIR:${target}>/shaders"
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${KAILUX_ASSETS_DIR}"
            "$<TARGET_FILE_DIR:${target}>/assets"
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${KAILUX_LICENSES_DIR}"
            "$<TARGET_FILE_DIR:${target}>/licenses"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${KAILUX_RUNTIME_LIBS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${KAILUX_RUNTIME_LIBS_DIR}"
            "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying shaders, assets, licenses and the OpenAL Soft library next to ${target}"
            VERBATIM
    )
    add_dependencies(${target} ${target}_resources)

    set_target_properties(${target} PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>"
            BUILD_RPATH "$ORIGIN"
            INSTALL_RPATH "$ORIGIN"
    )
endfunction()

option(KAILUX_ENABLE_PROFILING "Compile the CPU/GPU profiling scopes into the engine (needed by the benchmark sample)" OFF)

add_subdirectory(kailux)
add_subdirectory(prism)
add_subdirectory(spectrum)

option(KAILUX_BUILD_SAMPLES "Build the sample games" OFF)
if (KAILUX_BUILD_SAMPLES)
    add_subdirectory(samples)
endif ()