add_library(lob_core
    src/models/order.cpp
)

target_include_directories(lob_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(lob_core PUBLIC cxx_std_23)

add_subdirectory(test)