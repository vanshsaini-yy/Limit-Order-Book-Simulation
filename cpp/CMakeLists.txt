add_library(lob_core INTERFACE)

target_include_directories(lob_core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(lob_core INTERFACE cxx_std_23)
add_subdirectory(test)