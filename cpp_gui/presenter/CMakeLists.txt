# Presenter Layer - CMakeLists.txt
add_library(MachinaMindAIAgent_Presenter STATIC
    MainPresenter.cpp
    MainPresenter.h
)

target_link_libraries(MachinaMindAIAgent_Presenter PUBLIC
    Qt6::Core
    MachinaMindAIAgent_Model
)

target_include_directories(MachinaMindAIAgent_Presenter PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
