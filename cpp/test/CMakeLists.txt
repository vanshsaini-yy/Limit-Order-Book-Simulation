set(TEST_SOURCES
    models/test_order.cpp
    models/test_trade.cpp
    engine/test_limit_order_book.cpp
    engine/test_execution_engine.cpp
    engine/test_matching_engine_validation.cpp
    engine/test_matching_engine_match.cpp
    engine/test_matching_engine_stp.cpp
    engine/test_matching_engine_execution.cpp
    engine/test_matching_engine_tif.cpp
    engine/test_matching_engine_post_only.cpp
    engine/test_matching_engine_price_collar.cpp
    policy/test_order_lifecycle.cpp
    policy/test_order_validator.cpp
    policy/test_stp_policy.cpp
    utils/test_order_utils.cpp
    infra/test_binary_trade_logger.cpp
    infra/test_trade_id_generator.cpp
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