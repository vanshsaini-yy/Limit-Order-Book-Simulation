cmake_minimum_required(VERSION 3.25)
project(lob_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable testing
enable_testing()

# Add GoogleTest (vendored if present, otherwise fetch)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/googletest/CMakeLists.txt")
	add_subdirectory(googletest)
else()
	include(FetchContent)
	message(STATUS "googletest not found locally; fetching with FetchContent...")
	FetchContent_Declare(
		googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG v1.14.0
	)
	FetchContent_MakeAvailable(googletest)
endif()

add_subdirectory(cpp)