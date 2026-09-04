#include <gtest/gtest.h>
#include <memory>
#include "engine/matching_engine.hpp"

class MatchingEngineCancelIncomingSTPTest : public ::testing::Test {
protected:
    LimitOrderBook orderBook;
    CancelIncomingSTP cancelIncomingPolicy;
    MatchingEngine engineCancelIncoming{&orderBook, &cancelIncomingPolicy};
};

TEST_F(MatchingEngineCancelIncomingSTPTest, MatchOrder_SelfOrderOnSameSideAtSamePrice_DoesNotCancelAndBothRemainPending) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr incomingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547801);

    engineCancelIncoming.matchOrder(restingSelf);
    RejectionReason result = engineCancelIncoming.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelIncomingSTPTest, MatchOrder_SelfOrderOnSameSideAtDifferentPrice_DoesNotCancelAndBothRemainPending) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr incomingSelf = std::make_shared<Order>(2, 1, 101, 10, Side::Buy, OrderType::Limit, 1622547801);

    engineCancelIncoming.matchOrder(restingSelf);
    RejectionReason result = engineCancelIncoming.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelIncomingSTPTest, MatchOrder_IncomingLimitOrderSelfTrades_CancelsIncomingAndKeepsResting) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr incomingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547801);

    engineCancelIncoming.matchOrder(restingSelf);
    RejectionReason result = engineCancelIncoming.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelIncomingSTPTest, MatchOrder_IncomingLimitOrderFullyExecutesAgainstOtherOrderBeforeReachingSelfOrder_LeavesSelfOrderUntouched) {
    OrderPtr restingOther = std::make_shared<Order>(1, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547802);

    engineCancelIncoming.matchOrder(restingOther);
    engineCancelIncoming.matchOrder(restingSelf);
    RejectionReason result = engineCancelIncoming.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(0));
}

TEST_F(MatchingEngineCancelIncomingSTPTest, MatchOrder_IncomingLimitOrderSelfTradesAfterPartialFillAgainstOtherOrder_CancelsIncomingRemainderAndKeepsSelfOrderUntouched) {
    OrderPtr restingOther = std::make_shared<Order>(1, 2, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547802);

    engineCancelIncoming.matchOrder(restingOther);
    engineCancelIncoming.matchOrder(restingSelf);
    RejectionReason result = engineCancelIncoming.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(5));
}

TEST_F(MatchingEngineCancelIncomingSTPTest, MatchOrder_IncomingLimitOrderSelfTradesWithMultipleRestingSelfOrders_CancelsOnlyIncoming) {
    OrderPtr restingSelf1 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf2 = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547802);

    engineCancelIncoming.matchOrder(restingSelf1);
    engineCancelIncoming.matchOrder(restingSelf2);
    RejectionReason result = engineCancelIncoming.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_EQ(restingSelf1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf1->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingSelf2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf2->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelIncomingSTPTest, MatchOrder_IncomingMarketOrderSelfTrades_CancelsIncomingAndKeepsResting) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr incomingSelf = std::make_shared<Order>(2, 1, 0, 10, Side::Buy, OrderType::Market, 1622547801);

    engineCancelIncoming.matchOrder(restingSelf);
    RejectionReason result = engineCancelIncoming.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelIncomingSTPTest, MatchOrder_IncomingMarketOrderFullyExecutesAgainstOtherOrderBeforeReachingSelfOrder_LeavesSelfOrderUntouched) {
    OrderPtr restingOther = std::make_shared<Order>(1, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 0, 10, Side::Buy, OrderType::Market, 1622547802);

    engineCancelIncoming.matchOrder(restingOther);
    engineCancelIncoming.matchOrder(restingSelf);
    RejectionReason result = engineCancelIncoming.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(0));
}

TEST_F(MatchingEngineCancelIncomingSTPTest, MatchOrder_IncomingMarketOrderSelfTradesAfterPartialFillAgainstOtherOrder_CancelsIncomingRemainderAndKeepsSelfOrderUntouched) {
    OrderPtr restingOther = std::make_shared<Order>(1, 2, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 0, 10, Side::Buy, OrderType::Market, 1622547802);

    engineCancelIncoming.matchOrder(restingOther);
    engineCancelIncoming.matchOrder(restingSelf);
    RejectionReason result = engineCancelIncoming.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(5));
}

TEST_F(MatchingEngineCancelIncomingSTPTest, MatchOrder_IncomingMarketOrderSelfTradesWithMultipleRestingSelfOrders_CancelsOnlyIncoming) {
    OrderPtr restingSelf1 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf2 = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 0, 10, Side::Buy, OrderType::Market, 1622547802);

    engineCancelIncoming.matchOrder(restingSelf1);
    engineCancelIncoming.matchOrder(restingSelf2);
    RejectionReason result = engineCancelIncoming.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_EQ(restingSelf1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf1->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingSelf2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf2->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
}