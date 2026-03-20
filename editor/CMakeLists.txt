add_executable(kailux_editor
        src/main.cpp
)

add_dependencies(kailux_editor CompileShaders)

target_link_libraries(kailux_editor PRIVATE
        kailux_engine
)
