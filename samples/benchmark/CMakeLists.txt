set(SPHERE_SHOOTER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sphere_shooter")

add_executable(sphere_shooter_benchmark
        src/main.cpp
        src/BenchmarkLayer.cpp
        ${SPHERE_SHOOTER_DIR}/src/SphereShooter.cpp
        ${SPHERE_SHOOTER_DIR}/src/Bullet.cpp
)

kailux_copy_resources(sphere_shooter_benchmark)

add_custom_command(TARGET sphere_shooter_benchmark POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${SPHERE_SHOOTER_DIR}/scenes"
        "$<TARGET_FILE_DIR:sphere_shooter_benchmark>/scenes"
        COMMENT "Copying sphere_shooter scenes for the benchmark"
        VERBATIM
)

add_custom_command(TARGET sphere_shooter_benchmark POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${SPHERE_SHOOTER_DIR}/sounds"
        "$<TARGET_FILE_DIR:sphere_shooter_benchmark>/sounds"
        COMMENT "Copying sphere_shooter sounds for the benchmark"
        VERBATIM
)

target_precompile_headers(sphere_shooter_benchmark REUSE_FROM kailux)

target_link_libraries(sphere_shooter_benchmark PRIVATE
        kailux
)