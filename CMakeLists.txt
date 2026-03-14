cmake_minimum_required(VERSION 4.1)
project(kailux)

set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan 1.4 REQUIRED)
find_package(glfw3 REQUIRED)

add_subdirectory(engine)
add_subdirectory(editor)

