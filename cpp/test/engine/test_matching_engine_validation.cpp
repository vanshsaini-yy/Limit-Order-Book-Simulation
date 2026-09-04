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
    OrderPtr invalidLimitOrder6 = std::make_shared<Order>(5, 5, 100, 10, Side::None, OrderType::Limit, 1622547804);
    OrderPtr invalidLimitOrder7 = std::make_shared<Order>(0, 0, 100, 10, Side::Buy, OrderType::Limit, 1622547805);
    OrderPtr invalidLimitOrder8 = std::make_shared<Order>(7, 7, 100, 10, Side::Buy, OrderType::Limit, 1622547807, 1);

    EXPECT_EQ(engine.matchOrder(invalidLimitOrder1), RejectionReason::NullOrder);
    EXPECT_EQ(engine.matchOrder(invalidLimitOrder2), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine.matchOrder(invalidLimitOrder3), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine.matchOrder(invalidLimitOrder4), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine.matchOrder(invalidLimitOrder5), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine.matchOrder(invalidLimitOrder6), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine.matchOrder(invalidLimitOrder7), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine.matchOrder(invalidLimitOrder8), RejectionReason::InvalidLimitOrder);

    EXPECT_EQ(invalidLimitOrder2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder3->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder4->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder5->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder6->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder7->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder8->getStatus(), OrderStatus::Cancelled);
}

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_BadMarketOrders) {
    OrderPtr invalidMarketOrder1 = nullptr;
    OrderPtr invalidMarketOrder2 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Market, 1622547800);
    OrderPtr invalidMarketOrder3 = std::make_shared<Order>(2, 2, 0, 0, Side::Sell, OrderType::Market, 1622547801);
    OrderPtr invalidMarketOrder4 = std::make_shared<Order>(3, 3, 0, -10, Side::Sell, OrderType::Market, 1622547802);
    OrderPtr invalidMarketOrder5 = std::make_shared<Order>(4, 4, 0, 10, Side::None, OrderType::Market, 1622547803);
    OrderPtr invalidMarketOrder6 = std::make_shared<Order>(0, 0, 0, 10, Side::Sell, OrderType::Market, 1622547804);
    OrderPtr invalidMarketOrder7 = std::make_shared<Order>(6, 6, 0, 10, Side::Sell, OrderType::Market, 1622547805, 1);

    EXPECT_EQ(engine.matchOrder(invalidMarketOrder1), RejectionReason::NullOrder);
    EXPECT_EQ(engine.matchOrder(invalidMarketOrder2), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine.matchOrder(invalidMarketOrder3), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine.matchOrder(invalidMarketOrder4), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine.matchOrder(invalidMarketOrder5), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine.matchOrder(invalidMarketOrder6), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine.matchOrder(invalidMarketOrder7), RejectionReason::InvalidMarketOrder);

    EXPECT_EQ(invalidMarketOrder2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidMarketOrder3->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidMarketOrder4->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidMarketOrder5->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidMarketOrder6->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidMarketOrder7->getStatus(), OrderStatus::Cancelled);
}

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_BadCancelOrders) {
    OrderPtr invalidCancelOrder1 = nullptr;
    OrderPtr invalidCancelOrder2 = std::make_shared<Order>(1, 1, 100, 0, Side::None, OrderType::Cancel, 1622547800, 1);
    OrderPtr invalidCancelOrder3 = std::make_shared<Order>(2, 2, 0, 10, Side::None, OrderType::Cancel, 1622547801, 1);
    OrderPtr invalidCancelOrder4 = std::make_shared<Order>(3, 3, 0, 0, Side::Buy, OrderType::Cancel, 1622547802, 1);
    OrderPtr invalidCancelOrder5 = std::make_shared<Order>(4, 4, 0, 0, Side::Sell, OrderType::Cancel, 1622547803, 1);
    OrderPtr invalidCancelOrder6 = std::make_shared<Order>(0, 0, 0, 0, Side::None, OrderType::Cancel, 1622547804, 1);
    OrderPtr invalidCancelOrder7 = std::make_shared<Order>(5, 5, 0, 0, Side::None, OrderType::Cancel, 1622547805, 0);
    OrderPtr invalidCancelOrder8 = std::make_shared<Order>(6, 6, 0, 0, Side::None, OrderType::Cancel, 1622547806, 6);
    OrderPtr invalidCancelOrder9 = std::make_shared<Order>(7, 7, 0, 0, Side::None, OrderType::Cancel, 1622547807, 1, TimeInForce::IOC);
    OrderPtr invalidCancelOrder10 = std::make_shared<Order>(8, 8, 0, 0, Side::None, OrderType::Cancel, 1622547808, 1, TimeInForce::GTC, true);

    EXPECT_EQ(engine.matchOrder(invalidCancelOrder1), RejectionReason::NullOrder);
    EXPECT_EQ(engine.matchOrder(invalidCancelOrder2), RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(engine.matchOrder(invalidCancelOrder3), RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(engine.matchOrder(invalidCancelOrder4), RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(engine.matchOrder(invalidCancelOrder5), RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(engine.matchOrder(invalidCancelOrder6), RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(engine.matchOrder(invalidCancelOrder7), RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(engine.matchOrder(invalidCancelOrder8), RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(engine.matchOrder(invalidCancelOrder9), RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(engine.matchOrder(invalidCancelOrder10), RejectionReason::InvalidCancelOrder);

    EXPECT_EQ(invalidCancelOrder2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidCancelOrder3->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidCancelOrder4->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidCancelOrder5->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidCancelOrder6->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidCancelOrder7->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidCancelOrder8->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidCancelOrder9->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidCancelOrder10->getStatus(), OrderStatus::Cancelled);
}

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_BadStatusOrders) {
    OrderPtr order1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr order2 = std::make_shared<Order>(2, 2, 0, 10, Side::Sell, OrderType::Market, 1622547801);
    OrderPtr order3 = std::make_shared<Order>(3, 3, 0, 10, Side::Sell, OrderType::Market, 1622547802);
    OrderPtr order4 = std::make_shared<Order>(4, 4, 0, 0, Side::None, OrderType::Cancel, 1622547803, 1);

    order1->setStatus(OrderStatus::PartiallyExecuted);
    order2->setStatus(OrderStatus::Cancelled);
    order3->setStatus(OrderStatus::CancelledAfterPartialExecution);
    order4->setStatus(OrderStatus::Executed);

    EXPECT_EQ(engine.matchOrder(order1), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine.matchOrder(order2), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine.matchOrder(order3), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine.matchOrder(order4), RejectionReason::InvalidCancelOrder);

    EXPECT_EQ(order1->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(order2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(order3->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(order4->getStatus(), OrderStatus::Cancelled);
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

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_CancellingNonExistentOrder) {
    OrderID nonExistentOrderID = 999;
    OrderPtr cancelOrder = std::make_shared<Order>(1, 1, 0, 0, Side::None, OrderType::Cancel, 1622547801, nonExistentOrderID);

    EXPECT_EQ(engine.matchOrder(cancelOrder), RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_EQ(cancelOrder->getStatus(), OrderStatus::Cancelled);
}