#include <gtest/gtest.h>
#include <memory>
#include "engine/matching_engine.hpp"

class MatchingEngineCancelBothSTPTest : public ::testing::Test {
protected:
    LimitOrderBook orderBook;
    CancelBothSTP cancelBothPolicy;
    MatchingEngine engineCancelBoth{&orderBook, &cancelBothPolicy};
};

TEST_F(MatchingEngineCancelBothSTPTest, MatchOrder_SelfOrderOnSameSideAtSamePrice_DoesNotCancelAndBothRemainPending) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr incomingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547801);

    engineCancelBoth.matchOrder(restingSelf);
    RejectionReason result = engineCancelBoth.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelBothSTPTest, MatchOrder_SelfOrderOnSameSideAtDifferentPrice_DoesNotCancelAndBothRemainPending) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr incomingSelf = std::make_shared<Order>(2, 1, 101, 10, Side::Buy, OrderType::Limit, 1622547801);

    engineCancelBoth.matchOrder(restingSelf);
    RejectionReason result = engineCancelBoth.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelBothSTPTest, MatchOrder_IncomingLimitOrderSelfTrades_CancelsBothOrders) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr incomingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547801);

    engineCancelBoth.matchOrder(restingSelf);
    RejectionReason result = engineCancelBoth.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelBothSTPTest, MatchOrder_IncomingLimitOrderFullyExecutesAgainstOtherOrderBeforeReachingSelfOrder_LeavesSelfOrderUntouched) {
    OrderPtr restingOther = std::make_shared<Order>(1, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547802);

    engineCancelBoth.matchOrder(restingOther);
    engineCancelBoth.matchOrder(restingSelf);
    RejectionReason result = engineCancelBoth.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(0));
}

TEST_F(MatchingEngineCancelBothSTPTest, MatchOrder_IncomingLimitOrderSelfTradesAfterPartialFillAgainstOtherOrder_CancelsBothAndStopsIncoming) {
    OrderPtr restingOther = std::make_shared<Order>(1, 2, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547802);

    engineCancelBoth.matchOrder(restingOther);
    engineCancelBoth.matchOrder(restingSelf);
    RejectionReason result = engineCancelBoth.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(5));
}

TEST_F(MatchingEngineCancelBothSTPTest, MatchOrder_IncomingLimitOrderSelfTradesWithMultipleRestingSelfOrders_CancelsBothAndLeavesSecondSelfOrderUntouched) {
    OrderPtr restingSelf1 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf2 = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547802);

    engineCancelBoth.matchOrder(restingSelf1);
    engineCancelBoth.matchOrder(restingSelf2);
    RejectionReason result = engineCancelBoth.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_EQ(restingSelf1->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf1->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingSelf2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf2->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelBothSTPTest, MatchOrder_IncomingLimitOrderSelfTradesWithSelfOrderQueuedBeforeOtherOrder_CancelsBothAndLeavesOtherOrderUntouched) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingOther = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547802);

    engineCancelBoth.matchOrder(restingSelf);
    engineCancelBoth.matchOrder(restingOther);
    RejectionReason result = engineCancelBoth.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingOther->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingOther->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelBothSTPTest, MatchOrder_IncomingMarketOrderSelfTrades_CancelsBothOrders) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr incomingSelf = std::make_shared<Order>(2, 1, 0, 10, Side::Buy, OrderType::Market, 1622547801);

    engineCancelBoth.matchOrder(restingSelf);
    RejectionReason result = engineCancelBoth.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelBothSTPTest, MatchOrder_IncomingMarketOrderFullyExecutesAgainstOtherOrderBeforeReachingSelfOrder_LeavesSelfOrderUntouched) {
    OrderPtr restingOther = std::make_shared<Order>(1, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 0, 10, Side::Buy, OrderType::Market, 1622547802);

    engineCancelBoth.matchOrder(restingOther);
    engineCancelBoth.matchOrder(restingSelf);
    RejectionReason result = engineCancelBoth.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(0));
}

TEST_F(MatchingEngineCancelBothSTPTest, MatchOrder_IncomingMarketOrderSelfTradesAfterPartialFillAgainstOtherOrder_CancelsBothAndStopsIncoming) {
    OrderPtr restingOther = std::make_shared<Order>(1, 2, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 0, 10, Side::Buy, OrderType::Market, 1622547802);

    engineCancelBoth.matchOrder(restingOther);
    engineCancelBoth.matchOrder(restingSelf);
    RejectionReason result = engineCancelBoth.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(5));
}

TEST_F(MatchingEngineCancelBothSTPTest, MatchOrder_IncomingMarketOrderSelfTradesWithMultipleRestingSelfOrders_CancelsBothAndLeavesSecondSelfOrderUntouched) {
    OrderPtr restingSelf1 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf2 = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 0, 10, Side::Buy, OrderType::Market, 1622547802);

    engineCancelBoth.matchOrder(restingSelf1);
    engineCancelBoth.matchOrder(restingSelf2);
    RejectionReason result = engineCancelBoth.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_EQ(restingSelf1->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf1->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingSelf2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf2->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelBothSTPTest, MatchOrder_IncomingMarketOrderSelfTradesWithSelfOrderQueuedBeforeOtherOrder_CancelsBothAndLeavesOtherOrderUntouched) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingOther = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 0, 10, Side::Buy, OrderType::Market, 1622547802);

    engineCancelBoth.matchOrder(restingSelf);
    engineCancelBoth.matchOrder(restingOther);
    RejectionReason result = engineCancelBoth.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingOther->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingOther->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
}
