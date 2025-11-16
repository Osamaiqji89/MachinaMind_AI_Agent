# View Layer - CMakeLists.txt

# UI-Datei aus dem ui-Verzeichnis
set(UI_FILES
    ${CMAKE_SOURCE_DIR}/cpp_gui/ui/MainWindow.ui
)

# Qt UI-Suchpfade setzen
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/cpp_gui/ui)

add_library(MachinaMindAIAgent_View
    MainWindow.cpp
    MainWindow.h
    ${UI_FILES}
)

target_link_libraries(MachinaMindAIAgent_View PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
    MachinaMindAIAgent_Model
    MachinaMindAIAgent_Presenter
)

target_include_directories(MachinaMindAIAgent_View PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}  # Für generierte ui_*.h Dateien
)
