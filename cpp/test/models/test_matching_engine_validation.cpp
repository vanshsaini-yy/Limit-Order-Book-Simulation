#include <gtest/gtest.h>
#include <memory>
#include "models/matching_engine.hpp"

class MatchingEngineValidationTest : public ::testing::Test {
protected:
    LimitOrderBook* orderBook;
    STPPolicy* stpPolicy;
    MatchingEngine* engine;

    void SetUp() override {
        stpPolicy = new CancelBothSTP();
        orderBook = new LimitOrderBook();
        engine = new MatchingEngine(stpPolicy, orderBook);
    }

    void TearDown() override {
        delete engine;
        delete orderBook;
        delete stpPolicy;
    }
};

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_BadLimitOrders) {
    OrderPtr invalidLimitOrder1 = nullptr;
    OrderPtr invalidLimitOrder2 = new Order(1, 1, 0, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr invalidLimitOrder3 = new Order(2, 2, -100, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr invalidLimitOrder4 = new Order(3, 3, 100, 0, Side::Buy, OrderType::Limit, 1622547802);
    OrderPtr invalidLimitOrder5 = new Order(4, 4, 100, -10, Side::Buy, OrderType::Limit, 1622547803);
    OrderPtr invalidLimitOrder6 = new Order(5, 5, 100, 10, Side::None, OrderType::Limit, 1622547804);
    OrderPtr invalidLimitOrder7 = new Order(0, 0, 100, 10, Side::Buy, OrderType::Limit, 1622547805);
    OrderPtr invalidLimitOrder8 = new Order(7, 7, 100, 10, Side::Buy, OrderType::Limit, 1622547807, 1);

    EXPECT_EQ(engine->matchOrder(invalidLimitOrder1), RejectionReason::NullOrder);
    EXPECT_EQ(engine->matchOrder(invalidLimitOrder2), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine->matchOrder(invalidLimitOrder3), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine->matchOrder(invalidLimitOrder4), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine->matchOrder(invalidLimitOrder5), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine->matchOrder(invalidLimitOrder6), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine->matchOrder(invalidLimitOrder7), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine->matchOrder(invalidLimitOrder8), RejectionReason::InvalidLimitOrder);

    EXPECT_EQ(invalidLimitOrder2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder3->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder4->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder5->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder6->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder7->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidLimitOrder8->getStatus(), OrderStatus::Cancelled);

    delete invalidLimitOrder1;
    delete invalidLimitOrder2;
    delete invalidLimitOrder3;
    delete invalidLimitOrder4;
    delete invalidLimitOrder5;
    delete invalidLimitOrder6;
    delete invalidLimitOrder7;
    delete invalidLimitOrder8;
}

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_BadMarketOrders) {
    OrderPtr invalidMarketOrder1 = nullptr;
    OrderPtr invalidMarketOrder2 = new Order(1, 1, 100, 10, Side::Sell, OrderType::Market, 1622547800);
    OrderPtr invalidMarketOrder3 = new Order(2, 2, 0, 0, Side::Sell, OrderType::Market, 1622547801);
    OrderPtr invalidMarketOrder4 = new Order(3, 3, 0, -10, Side::Sell, OrderType::Market, 1622547802);
    OrderPtr invalidMarketOrder5 = new Order(4, 4, 0, 10, Side::None, OrderType::Market, 1622547803);
    OrderPtr invalidMarketOrder6 = new Order(0, 0, 0, 10, Side::Sell, OrderType::Market, 1622547804);
    OrderPtr invalidMarketOrder7 = new Order(6, 6, 0, 10, Side::Sell, OrderType::Market, 1622547807, 1);

    EXPECT_EQ(engine->matchOrder(invalidMarketOrder1), RejectionReason::NullOrder);
    EXPECT_EQ(engine->matchOrder(invalidMarketOrder2), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine->matchOrder(invalidMarketOrder3), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine->matchOrder(invalidMarketOrder4), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine->matchOrder(invalidMarketOrder5), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine->matchOrder(invalidMarketOrder6), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine->matchOrder(invalidMarketOrder7), RejectionReason::InvalidMarketOrder);

    EXPECT_EQ(invalidMarketOrder2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidMarketOrder3->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidMarketOrder4->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidMarketOrder5->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidMarketOrder6->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidMarketOrder7->getStatus(), OrderStatus::Cancelled);

    delete invalidMarketOrder1;
    delete invalidMarketOrder2;
    delete invalidMarketOrder3;
    delete invalidMarketOrder4;
    delete invalidMarketOrder5;
    delete invalidMarketOrder6;
    delete invalidMarketOrder7;
}

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_BadCancelOrders) {
    OrderPtr invalidCancelOrder1 = nullptr;
    OrderPtr invalidCancelOrder2 = new Order(1, 1, 100, 0, Side::None, OrderType::Cancel, 1622547800, 1);
    OrderPtr invalidCancelOrder3 = new Order(2, 2, 0, 10, Side::None, OrderType::Cancel, 1622547801, 1);
    OrderPtr invalidCancelOrder4 = new Order(3, 3, 0, 0, Side::Buy, OrderType::Cancel, 1622547802, 1);
    OrderPtr invalidCancelOrder5 = new Order(4, 4, 0, 0, Side::Sell, OrderType::Cancel, 1622547803, 1);
    OrderPtr invalidCancelOrder6 = new Order(0, 0, 0, 0, Side::None, OrderType::Cancel, 1622547804, 1);
    OrderPtr invalidCancelOrder7 = new Order(5, 5, 0, 0, Side::None, OrderType::Cancel, 1622547807, 0);
    OrderPtr invalidCancelOrder8 = new Order(6, 6, 0, 0, Side::None, OrderType::Cancel, 1622547807, 6);

    EXPECT_EQ(engine->matchOrder(invalidCancelOrder1), RejectionReason::NullOrder);
    EXPECT_EQ(engine->matchOrder(invalidCancelOrder2), RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(engine->matchOrder(invalidCancelOrder3), RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(engine->matchOrder(invalidCancelOrder4), RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(engine->matchOrder(invalidCancelOrder5), RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(engine->matchOrder(invalidCancelOrder6), RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(engine->matchOrder(invalidCancelOrder7), RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(engine->matchOrder(invalidCancelOrder8), RejectionReason::InvalidCancelOrder);

    EXPECT_EQ(invalidCancelOrder2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidCancelOrder3->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidCancelOrder4->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidCancelOrder5->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidCancelOrder6->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidCancelOrder7->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(invalidCancelOrder8->getStatus(), OrderStatus::Cancelled);

    delete invalidCancelOrder1;
    delete invalidCancelOrder2;
    delete invalidCancelOrder3;
    delete invalidCancelOrder4;
    delete invalidCancelOrder5;
    delete invalidCancelOrder6;
    delete invalidCancelOrder7;
    delete invalidCancelOrder8;
}

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_BadStatusOrders) {
    OrderPtr order1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr order2 = new Order(2, 2, 0, 10, Side::Sell, OrderType::Market, 1622547801);
    OrderPtr order3 = new Order(3, 3, 0, 10, Side::Sell, OrderType::Market, 1622547802);
    OrderPtr order4 = new Order(4, 4, 0, 0, Side::None, OrderType::Cancel, 1622547803, 1);

    order1->setStatus(OrderStatus::PartiallyExecuted);
    order2->setStatus(OrderStatus::Cancelled);
    order3->setStatus(OrderStatus::CancelledAfterPartialExecution);
    order4->setStatus(OrderStatus::Executed);

    EXPECT_EQ(engine->matchOrder(order1), RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(engine->matchOrder(order2), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine->matchOrder(order3), RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(engine->matchOrder(order4), RejectionReason::InvalidCancelOrder);

    EXPECT_EQ(order1->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(order2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(order3->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(order4->getStatus(), OrderStatus::Cancelled);

    delete order1;
    delete order2;
    delete order3;
    delete order4;
}

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_AddingDuplicateOrder) {
    OrderPtr order = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    engine->matchOrder(order);

    EXPECT_EQ(engine->matchOrder(order), RejectionReason::OrderToBeAddedAlreadyExists);
    EXPECT_EQ(order->getStatus(), OrderStatus::Pending);

    delete order;
}

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_AddingOrder_With_SameIDAsExistingOrder) {
    OrderPtr order1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr order2 = new Order(1, 2, 0, 20, Side::Sell, OrderType::Market, 1622547801);
    engine->matchOrder(order1);

    EXPECT_EQ(engine->matchOrder(order2), RejectionReason::OrderToBeAddedAlreadyExists);

    delete order1;
    delete order2;
}

TEST_F(MatchingEngineValidationTest, MatchingEngine_Rejects_CancellingNonExistentOrder) {
    OrderID nonExistentOrderID = 999;
    OrderPtr cancelOrder = new Order(1, 1, 0, 0, Side::None, OrderType::Cancel, 1622547801, nonExistentOrderID);

    EXPECT_EQ(engine->matchOrder(cancelOrder), RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_EQ(cancelOrder->getStatus(), OrderStatus::Cancelled);

    delete cancelOrder;
}