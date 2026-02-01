set(TEST_SOURCES
    models/test_order.cpp
)

add_executable(tests ${TEST_SOURCES})

target_link_libraries(tests
    PRIVATE
        lob_core
        gtest
        gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)