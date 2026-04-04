add_library(lob_core INTERFACE)

target_include_directories(lob_core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(lob_core INTERFACE cxx_std_23)

option(LOB_BUILD_PYTHON_BINDINGS "Build Python bindings module" ON)
option(LOB_FETCH_PYBIND11_IF_MISSING "Fetch pybind11 when not available on system" ON)

if(LOB_BUILD_PYTHON_BINDINGS)
    find_package(pybind11 CONFIG QUIET)

    if(NOT pybind11_FOUND AND LOB_FETCH_PYBIND11_IF_MISSING)
        include(FetchContent)
        message(STATUS "pybind11 not found; fetching with FetchContent...")

        FetchContent_Declare(
            pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11.git
            GIT_TAG v2.13.6
        )
        FetchContent_MakeAvailable(pybind11)
    endif()

    set(LOB_PYBIND11_READY OFF)
    if(pybind11_FOUND OR TARGET pybind11::module)
        set(LOB_PYBIND11_READY ON)
    endif()

    if(LOB_PYBIND11_READY AND COMMAND pybind11_add_module)
        pybind11_add_module(limit_order_book
            python/python_module.cpp
            python/order_bindings.cpp
            python/matching_engine_facade.cpp
            python/matching_engine_facade_bindings.cpp
        )

        target_link_libraries(limit_order_book
            PRIVATE
                lob_core
        )

        target_compile_features(limit_order_book PRIVATE cxx_std_23)
    else()
        message(STATUS "pybind11 not available for module build; skipping Python bindings target 'limit_order_book'.")
    endif()
endif()

add_subdirectory(test)