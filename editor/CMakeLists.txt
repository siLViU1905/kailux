add_executable(kailux_editor
        src/main.cpp
)

target_link_libraries(kailux_editor PRIVATE
        kailux_engine
)
