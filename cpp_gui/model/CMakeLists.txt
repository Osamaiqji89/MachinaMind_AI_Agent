# Model Layer - CMakeLists.txt
add_library(MachinaMindAIAgent_Model STATIC
    DataModel.cpp
    DataModel.h
    DTOs.h
    ApiClient.cpp
    ApiClient.h
)

target_link_libraries(MachinaMindAIAgent_Model PUBLIC
    Qt6::Core
    Qt6::Network
)

target_include_directories(MachinaMindAIAgent_Model PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
