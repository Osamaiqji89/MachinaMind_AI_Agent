cmake_minimum_required(VERSION 3.20)

project(MachinaMindAIAgent VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt Setup
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts Network)

# Build cpp_gui
add_subdirectory(cpp_gui)

# Uncomment when tests are ready
# add_subdirectory(tests)
