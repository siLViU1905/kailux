add_executable(prism
        src/panels/HierarchyPanel.cpp
        src/panels/Panel.cpp
        src/layers/EditorLayer.cpp
        src/panels/EntityEditorPanel.cpp
        src/panels/MenuPanel.cpp
        src/panels/project_panel/ProjectPanel.cpp
        src/panels/project_panel/AssetBrowser.cpp
        src/panels/project_panel/Console.cpp
        src/panels/ViewportPanel.cpp
        src/panels/SimulationPanel.cpp
        src/UIWidgets.cpp
        src/panels/PanelStack.cpp
        main.cpp
)

kailux_copy_resources(prism)

target_precompile_headers(prism REUSE_FROM kailux)

target_include_directories(prism PUBLIC src)

target_link_libraries(prism PRIVATE
        kailux
)
