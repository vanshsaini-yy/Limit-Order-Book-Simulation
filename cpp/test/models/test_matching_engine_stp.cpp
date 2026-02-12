#include <gtest/gtest.h>
#include <memory>
#include "models/matching_engine.hpp"

class MatchingEngineSTPTest : public ::testing::Test {
protected:
    LimitOrderBook* orderBook;
    STPPolicy* cancelBothPolicy;
    STPPolicy* cancelIncomingPolicy;
    STPPolicy* cancelRestingPolicy;
    MatchingEngine* engineCancelBoth;
    MatchingEngine* engineCancelIncoming;
    MatchingEngine* engineCancelResting;

    void SetUp() override {
        orderBook = new LimitOrderBook();

        cancelBothPolicy = new CancelBothSTP();
        engineCancelBoth = new MatchingEngine(cancelBothPolicy, orderBook);

        cancelIncomingPolicy = new CancelIncomingSTP();
        engineCancelIncoming = new MatchingEngine(cancelIncomingPolicy, orderBook);

        cancelRestingPolicy = new CancelRestingSTP();
        engineCancelResting = new MatchingEngine(cancelRestingPolicy, orderBook);
    }

    void TearDown() override {
        delete engineCancelBoth;
        delete engineCancelIncoming;
        delete engineCancelResting;
        delete orderBook;
        delete cancelBothPolicy;
        delete cancelIncomingPolicy;
        delete cancelRestingPolicy;
    }
};

TEST_F(MatchingEngineSTPTest, CancelBothSTP_CancelsIncoming_CancelsResting_OnSelfTrade) {
    OrderPtr restingOrder = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr incomingOrder = new Order(2, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547801);

    engineCancelBoth->matchOrder(restingOrder);
    engineCancelBoth->matchOrder(incomingOrder);

    EXPECT_EQ(incomingOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingOrder->getQty(), 10u);
    EXPECT_EQ(restingOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingOrder->getQty(), 10u);

    delete restingOrder;
    delete incomingOrder;
}

TEST_F(MatchingEngineSTPTest, CancelBothSTP_CancelsBoth_ForMarketIncomingSelfTrade) {
    OrderPtr restingOrder = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr incomingOrder = new Order(2, 1, 0, 10, Side::Buy, OrderType::Market, 1622547801);

    engineCancelBoth->matchOrder(restingOrder);
    engineCancelBoth->matchOrder(incomingOrder);

    EXPECT_EQ(incomingOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingOrder->getQty(), 10u);
    EXPECT_EQ(restingOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingOrder->getQty(), 10u);

    delete restingOrder;
    delete incomingOrder;
}

TEST_F(MatchingEngineSTPTest, CancelBothSTP_CancelsResting_CancelsIncomingAfterPartialFill) {
    OrderPtr restingOrder1 = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingOrder2 = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingOrder = new Order(3, 2, 100, 15, Side::Buy, OrderType::Limit, 1622547803);

    engineCancelBoth->matchOrder(restingOrder1);
    engineCancelBoth->matchOrder(restingOrder2);
    engineCancelBoth->matchOrder(incomingOrder);

    EXPECT_EQ(restingOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingOrder1->getQty(), 0u);
    EXPECT_EQ(restingOrder2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingOrder2->getQty(), 10u);
    EXPECT_EQ(incomingOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(incomingOrder->getQty(), 5u);

    delete restingOrder1;
    delete restingOrder2;
    delete incomingOrder;
}

TEST_F(MatchingEngineSTPTest, CancelBothSTP_AllowsExecution_WhenNoSelfTrade) {
    OrderPtr restingOrder1 = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingOrder2 = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingOrder = new Order(3, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547802);

    engineCancelBoth->matchOrder(restingOrder1);
    engineCancelBoth->matchOrder(restingOrder2);
    engineCancelBoth->matchOrder(incomingOrder);

    EXPECT_EQ(restingOrder1->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(restingOrder1->getQty(), 5u);
    EXPECT_EQ(restingOrder2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingOrder2->getQty(), 10u);
    EXPECT_EQ(incomingOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingOrder->getQty(), 0u);

    delete restingOrder1;
    delete restingOrder2;
    delete incomingOrder;
}

TEST_F(MatchingEngineSTPTest, CancelBothSTP_AllowsExecution_AgainstBestPrice) {
    OrderPtr restingOrder1 = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingOrder2 = new Order(2, 2, 101, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingOrder = new Order(3, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547802);

    engineCancelBoth->matchOrder(restingOrder1);
    engineCancelBoth->matchOrder(restingOrder2);
    engineCancelBoth->matchOrder(incomingOrder);

    EXPECT_EQ(restingOrder1->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(restingOrder1->getQty(), 5u);
    EXPECT_EQ(restingOrder2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingOrder2->getQty(), 10u);
    EXPECT_EQ(incomingOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingOrder->getQty(), 0u);

    delete restingOrder1;
    delete restingOrder2;
    delete incomingOrder;
}

TEST_F(MatchingEngineSTPTest, CancelBothSTP_ComplexSimulation) {
    OrderPtr restingOrder1 = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingOrder2 = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingOrder1 = new Order(3, 3, 100, 15, Side::Buy, OrderType::Limit, 1622547803);
    OrderPtr incomingOrder2 = new Order(4, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547804);

    engineCancelBoth->matchOrder(restingOrder1);
    engineCancelBoth->matchOrder(restingOrder2);
    engineCancelBoth->matchOrder(incomingOrder1);
    engineCancelBoth->matchOrder(incomingOrder2);

    EXPECT_EQ(restingOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingOrder1->getQty(), 0u);
    EXPECT_EQ(restingOrder2->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(restingOrder2->getQty(), 5u);
    EXPECT_EQ(incomingOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingOrder1->getQty(), 0u);
    EXPECT_EQ(incomingOrder2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingOrder2->getQty(), 10u);

    delete restingOrder1;
    delete restingOrder2;
    delete incomingOrder1;
    delete incomingOrder2;
}

TEST_F(MatchingEngineSTPTest, CancelIncomingSTP_CancelsIncoming_KeepsResting_OnSelfTrade) {
    OrderPtr restingOrder = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr incomingOrder = new Order(2, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547801);

    engineCancelIncoming->matchOrder(restingOrder);
    engineCancelIncoming->matchOrder(incomingOrder);

    EXPECT_EQ(incomingOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingOrder->getQty(), 10u);
    EXPECT_EQ(restingOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingOrder->getQty(), 10u);

    delete restingOrder;
    delete incomingOrder;
}

TEST_F(MatchingEngineSTPTest, CancelIncomingSTP_CancelsIncomingAfterPartialFill) {
    OrderPtr restingOrder1 = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingOrder2 = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingOrder = new Order(3, 2, 100, 15, Side::Buy, OrderType::Limit, 1622547803);

    engineCancelIncoming->matchOrder(restingOrder1);
    engineCancelIncoming->matchOrder(restingOrder2);
    engineCancelIncoming->matchOrder(incomingOrder);

    EXPECT_EQ(restingOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingOrder1->getQty(), 0u);
    EXPECT_EQ(restingOrder2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingOrder2->getQty(), 10u);
    EXPECT_EQ(incomingOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(incomingOrder->getQty(), 5u);

    delete restingOrder1;
    delete restingOrder2;
    delete incomingOrder;
}

TEST_F(MatchingEngineSTPTest, CancelIncomingSTP_AllowsExecution_WhenNoSelfTrade) {
    OrderPtr restingOrder1 = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingOrder2 = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingOrder = new Order(3, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547803);

    engineCancelIncoming->matchOrder(restingOrder1);
    engineCancelIncoming->matchOrder(restingOrder2);
    engineCancelIncoming->matchOrder(incomingOrder);

    EXPECT_EQ(restingOrder1->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(restingOrder1->getQty(), 5u);
    EXPECT_EQ(restingOrder2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingOrder2->getQty(), 10u);
    EXPECT_EQ(incomingOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingOrder->getQty(), 0u);

    delete restingOrder1;
    delete restingOrder2;
    delete incomingOrder;
}

TEST_F(MatchingEngineSTPTest, CancelIncomingSTP_AllowsExecution_AgainstBestPrice) {
    OrderPtr restingOrder1 = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingOrder2 = new Order(2, 2, 101, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingOrder = new Order(3, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547803);

    engineCancelIncoming->matchOrder(restingOrder1);
    engineCancelIncoming->matchOrder(restingOrder2);
    engineCancelIncoming->matchOrder(incomingOrder);

    EXPECT_EQ(restingOrder1->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(restingOrder1->getQty(), 5u);
    EXPECT_EQ(restingOrder2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingOrder2->getQty(), 10u);
    EXPECT_EQ(incomingOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingOrder->getQty(), 0u);

    delete restingOrder1;
    delete restingOrder2;
    delete incomingOrder;
}

TEST_F(MatchingEngineSTPTest, CancelIncomingSTP_CancelsMarketIncoming_KeepsResting) {
    OrderPtr restingOrder = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr incomingOrder = new Order(2, 1, 0, 10, Side::Buy, OrderType::Market, 1622547801);

    engineCancelIncoming->matchOrder(restingOrder);
    engineCancelIncoming->matchOrder(incomingOrder);

    EXPECT_EQ(incomingOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingOrder->getQty(), 10u);
    EXPECT_EQ(restingOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingOrder->getQty(), 10u);

    delete restingOrder;
    delete incomingOrder;
}

TEST_F(MatchingEngineSTPTest, CancelRestingSTP_CancelsResting_KeepsIncoming_OnSelfTrade) {
    OrderPtr restingOrder = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr incomingOrder = new Order(2, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547801);

    engineCancelResting->matchOrder(restingOrder);
    engineCancelResting->matchOrder(incomingOrder);

    EXPECT_EQ(incomingOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(incomingOrder->getQty(), 10u);
    EXPECT_EQ(restingOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingOrder->getQty(), 10u);

    delete restingOrder;
    delete incomingOrder;
}

TEST_F(MatchingEngineSTPTest, CancelRestingSTP_CancelsBothForMarketIncoming) {
    OrderPtr restingOrder = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr incomingOrder = new Order(2, 1, 0, 10, Side::Buy, OrderType::Market, 1622547801);

    engineCancelResting->matchOrder(restingOrder);
    engineCancelResting->matchOrder(incomingOrder);

    EXPECT_EQ(incomingOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingOrder->getQty(), 10u);
    EXPECT_EQ(restingOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingOrder->getQty(), 10u);

    delete restingOrder;
    delete incomingOrder;
}

TEST_F(MatchingEngineSTPTest, CancelRestingSTP_CancelsSelfResting_ThenContinuesMatching) {
    OrderPtr restingOrder1 = new Order(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingOrder2 = new Order(2, 2, 100, 7, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingOrder = new Order(3, 1, 100, 6, Side::Buy, OrderType::Limit, 1622547802);

    engineCancelResting->matchOrder(restingOrder1);
    engineCancelResting->matchOrder(restingOrder2);
    engineCancelResting->matchOrder(incomingOrder);

    EXPECT_EQ(restingOrder1->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingOrder1->getQty(), 5u);
    EXPECT_EQ(restingOrder2->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(restingOrder2->getQty(), 1u);
    EXPECT_EQ(incomingOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingOrder->getQty(), 0u);

    delete restingOrder1;
    delete restingOrder2;
    delete incomingOrder;
}

TEST_F(MatchingEngineSTPTest, CancelRestingSTP_CancelsResting_ThenAllowsExecution) {
    OrderPtr restingOrder1 = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingOrder2 = new Order(2, 2, 100, 5, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingOrder = new Order(3, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547803);

    engineCancelResting->matchOrder(restingOrder1);
    engineCancelResting->matchOrder(restingOrder2);
    engineCancelResting->matchOrder(incomingOrder);

    EXPECT_EQ(restingOrder1->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingOrder1->getQty(), 10u);
    EXPECT_EQ(restingOrder2->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingOrder2->getQty(), 0u);
    EXPECT_EQ(incomingOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingOrder->getQty(), 0u);

    delete restingOrder1;
    delete restingOrder2;
    delete incomingOrder;
}