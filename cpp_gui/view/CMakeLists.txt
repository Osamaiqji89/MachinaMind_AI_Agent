# View Layer - CMakeLists.txt
add_library(MachinaMindAIAgent_View
    MainWindow.cpp
    MainWindow.h
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
)
