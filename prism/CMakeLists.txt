add_executable(prism
        src/core/panels/HierarchyPanel.cpp
        src/core/panels/Panel.cpp
        src/core/layers/EditorLayer.cpp
        src/core/panels/EntityEditorPanel.cpp
        src/core/panels/MenuPanel.cpp
        src/core/panels/project_panel/ProjectPanel.cpp
        src/core/panels/project_panel/AssetBrowser.cpp
        src/core/panels/project_panel/Console.cpp
        src/core/panels/ViewportPanel.cpp
        src/core/panels/SimulationPanel.cpp
        src/core/UIWidgets.cpp
        src/core/panels/PanelStack.cpp
        main.cpp
)

kailux_copy_resources(prism)

target_precompile_headers(prism REUSE_FROM kailux)

target_include_directories(prism PUBLIC src)

target_link_libraries(prism PRIVATE
        kailux
)
