#include <gtest/gtest.h>
#include <memory>
#include "engine/matching_engine.hpp"

class MatchingEngineCancelRestingSTPTest : public ::testing::Test {
protected:
    LimitOrderBook orderBook;
    CancelRestingSTP cancelRestingPolicy;
    MatchingEngine engineCancelResting{&orderBook, &cancelRestingPolicy};
};

TEST_F(MatchingEngineCancelRestingSTPTest, MatchOrder_SelfOrderOnSameSideAtSamePrice_DoesNotCancelAndBothRemainPending) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr incomingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547801);

    engineCancelResting.matchOrder(restingSelf);
    RejectionReason result = engineCancelResting.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelRestingSTPTest, MatchOrder_SelfOrderOnSameSideAtDifferentPrice_DoesNotCancelAndBothRemainPending) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr incomingSelf = std::make_shared<Order>(2, 1, 101, 10, Side::Buy, OrderType::Limit, 1622547801);

    engineCancelResting.matchOrder(restingSelf);
    RejectionReason result = engineCancelResting.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelRestingSTPTest, MatchOrder_IncomingLimitOrderSelfTrades_CancelsRestingAndKeepsIncomingPending) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr incomingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547801);

    engineCancelResting.matchOrder(restingSelf);
    RejectionReason result = engineCancelResting.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelRestingSTPTest, MatchOrder_IncomingLimitOrderSelfTradesWithFirstRestingOrder_CancelsFirstRestingThenKeepsExecuting) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingOther = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547802);

    engineCancelResting.matchOrder(restingSelf);
    engineCancelResting.matchOrder(restingOther);
    RejectionReason result = engineCancelResting.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingOther->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingOther->getQty(), static_cast<Quantity>(0));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(0));
}

TEST_F(MatchingEngineCancelRestingSTPTest, MatchOrder_IncomingLimitOrderSelfTradesWithSecondRestingOrder_CancelsSecondRestingAndFullyExecutesAgainstFirstAndThirdRestingOrders) {
    OrderPtr restingOther1 = std::make_shared<Order>(1, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr restingOther2 = std::make_shared<Order>(3, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547802);
    OrderPtr incomingSelf = std::make_shared<Order>(4, 1, 100, 20, Side::Buy, OrderType::Limit, 1622547803);

    engineCancelResting.matchOrder(restingOther1);
    engineCancelResting.matchOrder(restingSelf);
    engineCancelResting.matchOrder(restingOther2);
    RejectionReason result = engineCancelResting.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(0));
}

TEST_F(MatchingEngineCancelRestingSTPTest, MatchOrder_IncomingLimitOrderSelfTradesWithMultipleRestingOrders_CancelsAllAndKeepsIncomingPending) {
    OrderPtr restingSelf1 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf2 = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547802);

    engineCancelResting.matchOrder(restingSelf1);
    engineCancelResting.matchOrder(restingSelf2);
    RejectionReason result = engineCancelResting.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf1->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf1->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingSelf2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf2->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelRestingSTPTest, MatchOrder_IncomingLimitOrderFullyExecutesAtBestPriceBeforeReachingSelfOrderDeeperInBook_LeavesSelfOrderUntouched) {
    OrderPtr restingOther = std::make_shared<Order>(1, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf = std::make_shared<Order>(2, 1, 101, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 101, 10, Side::Buy, OrderType::Limit, 1622547802);

    engineCancelResting.matchOrder(restingOther);
    engineCancelResting.matchOrder(restingSelf);
    RejectionReason result = engineCancelResting.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingOther->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingOther->getQty(), static_cast<Quantity>(0));
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(0));
}

TEST_F(MatchingEngineCancelRestingSTPTest, MatchOrder_IncomingMarketOrderSelfTrades_CancelsRestingAndDiscardsIncoming) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr incomingSelf = std::make_shared<Order>(2, 1, 0, 10, Side::Buy, OrderType::Market, 1622547801);

    engineCancelResting.matchOrder(restingSelf);
    RejectionReason result = engineCancelResting.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelRestingSTPTest, MatchOrder_IncomingMarketOrderSelfTradesWithFirstRestingOrder_CancelsFirstRestingThenKeepsExecuting) {
    OrderPtr restingSelf = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingOther = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 0, 10, Side::Buy, OrderType::Market, 1622547802);

    engineCancelResting.matchOrder(restingSelf);
    engineCancelResting.matchOrder(restingOther);
    RejectionReason result = engineCancelResting.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingOther->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingOther->getQty(), static_cast<Quantity>(0));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(0));
}

TEST_F(MatchingEngineCancelRestingSTPTest, MatchOrder_IncomingMarketOrderSelfTradesWithSecondRestingOrder_CancelsSecondRestingAndFullyExecutesAgainstFirstAndThirdRestingOrders) {
    OrderPtr restingOther1 = std::make_shared<Order>(1, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr restingOther2 = std::make_shared<Order>(3, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547802);
    OrderPtr incomingSelf = std::make_shared<Order>(4, 1, 0, 20, Side::Buy, OrderType::Market, 1622547803);

    engineCancelResting.matchOrder(restingOther1);
    engineCancelResting.matchOrder(restingSelf);
    engineCancelResting.matchOrder(restingOther2);
    RejectionReason result = engineCancelResting.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(0));
}

TEST_F(MatchingEngineCancelRestingSTPTest, MatchOrder_IncomingMarketOrderSelfTradesWithMultipleRestingOrders_CancelsAllAndDiscardsIncoming) {
    OrderPtr restingSelf1 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf2 = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 0, 10, Side::Buy, OrderType::Market, 1622547802);

    engineCancelResting.matchOrder(restingSelf1);
    engineCancelResting.matchOrder(restingSelf2);
    RejectionReason result = engineCancelResting.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSelf1->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf1->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(restingSelf2->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(restingSelf2->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineCancelRestingSTPTest, MatchOrder_IncomingMarketOrderFullyExecutesAtBestPriceBeforeReachingSelfOrderDeeperInBook_LeavesSelfOrderUntouched) {
    OrderPtr restingOther = std::make_shared<Order>(1, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr restingSelf = std::make_shared<Order>(2, 1, 101, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr incomingSelf = std::make_shared<Order>(3, 1, 0, 10, Side::Buy, OrderType::Market, 1622547802);

    engineCancelResting.matchOrder(restingOther);
    engineCancelResting.matchOrder(restingSelf);
    RejectionReason result = engineCancelResting.matchOrder(incomingSelf);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingOther->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingOther->getQty(), static_cast<Quantity>(0));
    EXPECT_EQ(restingSelf->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSelf->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(incomingSelf->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(incomingSelf->getQty(), static_cast<Quantity>(0));
}
