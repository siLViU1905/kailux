add_library(kailux_editor
        src/panels/HierarchyPanel.cpp
        src/panels/Panel.cpp
        src/layers/Layer.cpp
        src/Editor.cpp
        src/layers/EditorLayer.cpp
        src/panels/EntityEditorPanel.cpp
        src/panels/MenuPanel.cpp
)

target_precompile_headers(kailux_editor REUSE_FROM kailux_engine)

target_include_directories(kailux_editor PUBLIC src)

target_link_libraries(kailux_editor PRIVATE
        kailux_engine
)
