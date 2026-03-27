add_executable(kailux_editor
        src/main.cpp
)

target_precompile_headers(kailux_editor REUSE_FROM kailux_engine)

add_dependencies(kailux_editor CompileShaders)

target_link_libraries(kailux_editor PRIVATE
        kailux_engine
)
