add_library(kailux_editor
        src/core/panels/HierarchyPanel.cpp
        src/core/panels/Panel.cpp
        src/core/layers/Layer.cpp
        src/core/Editor.cpp
        src/core/layers/EditorLayer.cpp
        src/core/panels/EntityEditorPanel.cpp
        src/core/panels/MenuPanel.cpp
)

target_precompile_headers(kailux_editor REUSE_FROM kailux_engine)

target_include_directories(kailux_editor PUBLIC src)

target_link_libraries(kailux_editor PRIVATE
        kailux_engine
)
