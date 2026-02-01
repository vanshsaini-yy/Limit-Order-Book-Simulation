add_executable(test_order
    models/test_order.cpp
)

target_link_libraries(test_order
    PRIVATE
        lob_core
        gtest
        gtest_main
)

add_test(NAME OrderTests COMMAND test_order)