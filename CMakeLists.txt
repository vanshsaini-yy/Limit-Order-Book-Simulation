cmake_minimum_required(VERSION 3.25)
project(lob_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable testing
enable_testing()

# Add subprojects
add_subdirectory(googletest)
add_subdirectory(cpp)