set(TEST_SOURCES
    models/test_order.cpp
    models/test_order_book.cpp
    models/test_execution_engine.cpp
    utils/test_order_utils.cpp
    models/test_matching_engine_match.cpp
    models/test_matching_engine_stp.cpp
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