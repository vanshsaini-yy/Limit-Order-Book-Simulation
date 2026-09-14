#include <gtest/gtest.h>
#include <memory>
#include "engine/matching_engine.hpp"

class MatchingEngineValidationTest : public ::testing::Test {
protected:
    CancelBothSTP stpPolicy;
    LimitOrderBook orderBook;
    MatchingEngine engine{&orderBook, &stpPolicy};
};

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_BadLimitOrders) {
    OrderPtr invalidLimitOrder1 = nullptr;
    OrderPtr invalidLimitOrder2 = std::make_shared<Order>(1, 1, 0, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr invalidLimitOrder3 = std::make_shared<Order>(2, 2, -100, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr invalidLimitOrder4 = std::make_shared<Order>(3, 3, 100, 0, Side::Buy, OrderType::Limit, 1622547802);
    OrderPtr invalidLimitOrder5 = std::make_shared<Order>(4, 4, 100, -10, Side::Buy, OrderType::Limit, 1622547803);
    OrderPtr invalidLimitOrder6 = std::make_shared<Order>(0, 0, 100, 10, Side::Buy, OrderType::Limit, 1622547805);

    EXPECT_EQ(engine.matchOrder(invalidLimitOrder1), RejectionReason::NullOrder);
    EXPECT_EQ(engine.matchOrder(invalidLimitOrder2), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine.matchOrder(invalidLimitOrder3), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine.matchOrder(invalidLimitOrder4), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine.matchOrder(invalidLimitOrder5), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine.matchOrder(invalidLimitOrder6), RejectionReason::InvalidLimitOrder);

    EXPECT_EQ(invalidLimitOrder2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder3->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder4->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder5->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder6->getStatus(), OrderStatus::Cancelled);
}

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_BadMarketOrders) {
    OrderPtr invalidMarketOrder1 = nullptr;
    OrderPtr invalidMarketOrder2 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Market, 1622547800);
    OrderPtr invalidMarketOrder3 = std::make_shared<Order>(2, 2, 0, 0, Side::Sell, OrderType::Market, 1622547801);
    OrderPtr invalidMarketOrder4 = std::make_shared<Order>(3, 3, 0, -10, Side::Sell, OrderType::Market, 1622547802);
    OrderPtr invalidMarketOrder5 = std::make_shared<Order>(0, 0, 0, 10, Side::Sell, OrderType::Market, 1622547804);

    EXPECT_EQ(engine.matchOrder(invalidMarketOrder1), RejectionReason::NullOrder);
    EXPECT_EQ(engine.matchOrder(invalidMarketOrder2), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine.matchOrder(invalidMarketOrder3), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine.matchOrder(invalidMarketOrder4), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine.matchOrder(invalidMarketOrder5), RejectionReason::InvalidMarketOrder);

    EXPECT_EQ(invalidMarketOrder2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidMarketOrder3->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidMarketOrder4->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidMarketOrder5->getStatus(), OrderStatus::Cancelled);
}

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_BadStatusOrders) {
    OrderPtr order1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr order2 = std::make_shared<Order>(2, 2, 0, 10, Side::Sell, OrderType::Market, 1622547801);
    OrderPtr order3 = std::make_shared<Order>(3, 3, 0, 10, Side::Sell, OrderType::Market, 1622547802);

    order1->setStatus(OrderStatus::PartiallyExecuted);
    order2->setStatus(OrderStatus::Cancelled);
    order3->setStatus(OrderStatus::CancelledAfterPartialExecution);

    EXPECT_EQ(engine.matchOrder(order1), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine.matchOrder(order2), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine.matchOrder(order3), RejectionReason::InvalidMarketOrder);

    EXPECT_EQ(order1->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(order2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(order3->getStatus(), OrderStatus::Cancelled);
}

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_AddingDuplicateOrder) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    engine.matchOrder(order);

    EXPECT_EQ(engine.matchOrder(order), RejectionReason::DuplicateOrderID);
    EXPECT_EQ(order->getStatus(), OrderStatus::Pending);
}

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_AddingOrder_With_SameIDAsExistingOrder) {
    OrderPtr order1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr order2 = std::make_shared<Order>(1, 2, 0, 20, Side::Sell, OrderType::Market, 1622547801);
    engine.matchOrder(order1);

    EXPECT_EQ(engine.matchOrder(order2), RejectionReason::DuplicateOrderID);
}