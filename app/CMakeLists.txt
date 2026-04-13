add_executable(kailux_application
        src/Application.cpp
        src/main.cpp
)

target_precompile_headers(kailux_application REUSE_FROM kailux_engine)

add_dependencies(kailux_application CompileShaders CopyAssets)

target_link_libraries(kailux_application PRIVATE
        kailux_engine
        kailux_editor
)