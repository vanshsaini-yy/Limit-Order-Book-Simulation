add_executable(test_order
    models/test_order.cpp
)

target_link_libraries(test_order
    PRIVATE
        lob_core
        gtest
        gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_order)