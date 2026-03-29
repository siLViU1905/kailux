add_executable(kailux_editor
        src/main.cpp
        src/panels/HierarchyPanel.cpp
        src/panels/Panel.cpp
        src/layers/Layer.cpp
        src/Editor.cpp
        src/layers/EditorLayer.cpp
)

target_precompile_headers(kailux_editor REUSE_FROM kailux_engine)

add_dependencies(kailux_editor CompileShaders)

target_link_libraries(kailux_editor PRIVATE
        kailux_engine
)
