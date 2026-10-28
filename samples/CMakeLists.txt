add_subdirectory(sphere_shooter)

if (KAILUX_ENABLE_PROFILING)
    add_subdirectory(benchmark)
else ()
    message(STATUS "sphere_shooter_benchmark skipped, configure with -DKAILUX_ENABLE_PROFILING=ON to build it")
endif ()