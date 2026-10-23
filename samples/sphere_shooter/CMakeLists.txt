add_executable(sphere_shooter
        src/main.cpp
        src/SphereShooter.cpp
        src/Bullet.cpp
)

kailux_copy_resources(sphere_shooter)

add_custom_command(TARGET sphere_shooter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/scenes"
        "$<TARGET_FILE_DIR:sphere_shooter>/scenes"
        COMMENT "Copying sphere_shooter scenes"
        VERBATIM
)

add_custom_command(TARGET sphere_shooter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/sounds"
        "$<TARGET_FILE_DIR:sphere_shooter>/sounds"
        COMMENT "Copying sphere_shooter sounds"
        VERBATIM
)

target_precompile_headers(sphere_shooter REUSE_FROM kailux)

target_link_libraries(sphere_shooter PRIVATE
        kailux
)