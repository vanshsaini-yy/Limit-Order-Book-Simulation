#include <gtest/gtest.h>
#include "engine/matching_engine.hpp"
#include "support/cancel_request.hpp"
#include "support/new_order.hpp"

class MatchingEngineCancelTest : public ::testing::Test {
protected:
    CancelBothSTP stpPolicy;
    LimitOrderBook orderBook;
    MatchingEngine engine{&orderBook, &stpPolicy};
};

// =====================================================================
// Request validation
// =====================================================================

TEST_F(MatchingEngineCancelTest, Submit_CancelWithZeroTargetOrderID_RejectedAsInvalidCancelOrder) {
    CancelRequest request = cancelRequestHelper(0, 1, 1000);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(0));
}

// =====================================================================
// Order lookup
// =====================================================================

TEST_F(MatchingEngineCancelTest, Submit_CancelNonExistentOrder_RejectedAsOrderToBeCancelledDoesNotExist) {
    OrderID nonExistentOrderID = 999;
    CancelRequest cancelRequest = cancelRequestHelper(nonExistentOrderID, 1, 1000);

    SubmitResult result = engine.submit(cancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(0));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelAnotherOwnersOrder_RejectedAsOrderToBeCancelledDoesNotExist) {
    OrderID restingOrderID = 1;
    OwnerID restingOwnerID = 1;
    OwnerID anotherOwnerID = 2;
    NewOrderRequest restingBuy = newOrderRequestHelper(restingOrderID, restingOwnerID, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr restingOrder = engine.submit(restingBuy).order;
    CancelRequest cancelRequest = cancelRequestHelper(restingOrderID, anotherOwnerID, 1001);

    SubmitResult result = engine.submit(cancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_EQ(restingOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingOrder->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(restingOrderID));
    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(0));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelAnotherOwnersOrder_IndistinguishableFromNonExistentOrder) {
    OrderID restingOrderID = 1;
    OwnerID restingOwnerID = 1;
    OwnerID anotherOwnerID = 2;
    OrderID nonExistentOrderID = 999;
    NewOrderRequest restingBuy = newOrderRequestHelper(restingOrderID, restingOwnerID, 100, 10, Side::Buy, OrderType::Limit, 1000);
    CancelRequest wrongOwnerCancel = cancelRequestHelper(restingOrderID, anotherOwnerID, 1001);
    CancelRequest nonExistentCancel = cancelRequestHelper(nonExistentOrderID, anotherOwnerID, 1002);
    engine.submit(restingBuy);

    SubmitResult wrongOwnerResult = engine.submit(wrongOwnerCancel);
    SubmitResult nonExistentResult = engine.submit(nonExistentCancel);

    EXPECT_EQ(wrongOwnerResult.reason, nonExistentResult.reason);
    EXPECT_EQ(wrongOwnerResult.order, nonExistentResult.order);
    EXPECT_EQ(wrongOwnerResult.order, nullptr);
}

TEST_F(MatchingEngineCancelTest, Submit_CancelAlreadyCancelledOrder_RejectedAsOrderToBeCancelledDoesNotExist) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    CancelRequest firstCancel = cancelRequestHelper(1, 1, 1001);
    CancelRequest secondCancel = cancelRequestHelper(1, 1, 1002);
    engine.submit(restingBuy);
    engine.submit(firstCancel);

    SubmitResult result = engine.submit(secondCancel);

    EXPECT_EQ(result.reason, RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(1));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelFullyFilledOrder_RejectedAsOrderToBeCancelledDoesNotExist) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest fillingSell = newOrderRequestHelper(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    CancelRequest cancelRequest = cancelRequestHelper(1, 1, 1002);
    OrderPtr filledOrder = engine.submit(restingBuy).order;
    engine.submit(fillingSell);

    SubmitResult result = engine.submit(cancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_EQ(filledOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(0));
}

// =====================================================================
// Successful cancel
// =====================================================================

TEST_F(MatchingEngineCancelTest, Submit_CancelRestingBuy_CancelsOrder) {
    OrderID restingOrderID = 1;
    NewOrderRequest restingBuy = newOrderRequestHelper(restingOrderID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    CancelRequest cancelRequest = cancelRequestHelper(restingOrderID, 1, 1001);
    OrderPtr restingOrder = engine.submit(restingBuy).order;

    SubmitResult result = engine.submit(cancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order, restingOrder);
    EXPECT_EQ(restingOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(restingOrderID));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelRestingSell_CancelsOrder) {
    OrderID restingOrderID = 1;
    NewOrderRequest restingSell = newOrderRequestHelper(restingOrderID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    CancelRequest cancelRequest = cancelRequestHelper(restingOrderID, 1, 1001);
    OrderPtr restingOrder = engine.submit(restingSell).order;

    SubmitResult result = engine.submit(cancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order, restingOrder);
    EXPECT_EQ(restingOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(restingOrderID));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelOneOfSeveralBuysAtSameLevel_OtherBuysStayPending) {
    OrderID firstOrderID = 1;
    OrderID secondOrderID = 2;
    OrderID thirdOrderID = 3;
    NewOrderRequest firstBuy = newOrderRequestHelper(firstOrderID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondBuy = newOrderRequestHelper(secondOrderID, 1, 100, 10, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest thirdBuy = newOrderRequestHelper(thirdOrderID, 1, 100, 10, Side::Buy, OrderType::Limit, 1002);
    CancelRequest cancelRequest = cancelRequestHelper(secondOrderID, 1, 1003);
    OrderPtr firstOrder = engine.submit(firstBuy).order;
    OrderPtr secondOrder = engine.submit(secondBuy).order;
    OrderPtr thirdOrder = engine.submit(thirdBuy).order;

    SubmitResult result = engine.submit(cancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(secondOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(secondOrderID));
    EXPECT_EQ(firstOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(firstOrderID));
    EXPECT_EQ(thirdOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(thirdOrderID));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelOneOfSeveralSellsAtSameLevel_OtherSellsStayPending) {
    OrderID firstOrderID = 1;
    OrderID secondOrderID = 2;
    OrderID thirdOrderID = 3;
    NewOrderRequest firstSell = newOrderRequestHelper(firstOrderID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondSell = newOrderRequestHelper(secondOrderID, 1, 100, 10, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest thirdSell = newOrderRequestHelper(thirdOrderID, 1, 100, 10, Side::Sell, OrderType::Limit, 1002);
    CancelRequest cancelRequest = cancelRequestHelper(secondOrderID, 1, 1003);
    OrderPtr firstOrder = engine.submit(firstSell).order;
    OrderPtr secondOrder = engine.submit(secondSell).order;
    OrderPtr thirdOrder = engine.submit(thirdSell).order;

    SubmitResult result = engine.submit(cancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(secondOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(secondOrderID));
    EXPECT_EQ(firstOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(firstOrderID));
    EXPECT_EQ(thirdOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(thirdOrderID));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelBestBid_NextBidBecomesBestBid) {
    OrderID bestBidOrderID = 1;
    OrderID nextBidOrderID = 2;
    PriceTicks nextBidPrice = 99;
    NewOrderRequest bestBid = newOrderRequestHelper(bestBidOrderID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest nextBid = newOrderRequestHelper(nextBidOrderID, 1, nextBidPrice, 10, Side::Buy, OrderType::Limit, 1001);
    CancelRequest cancelRequest = cancelRequestHelper(bestBidOrderID, 1, 1002);
    engine.submit(bestBid);
    engine.submit(nextBid);

    SubmitResult result = engine.submit(cancelRequest);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1003);
    EXPECT_EQ(result.reason, RejectionReason::None);
    ASSERT_TRUE(snapshot.bestBid.has_value());
    EXPECT_EQ(*snapshot.bestBid, nextBidPrice);
}

TEST_F(MatchingEngineCancelTest, Submit_CancelBestAsk_NextAskBecomesBestAsk) {
    OrderID bestAskOrderID = 1;
    OrderID nextAskOrderID = 2;
    PriceTicks nextAskPrice = 101;
    NewOrderRequest bestAsk = newOrderRequestHelper(bestAskOrderID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest nextAsk = newOrderRequestHelper(nextAskOrderID, 1, nextAskPrice, 10, Side::Sell, OrderType::Limit, 1001);
    CancelRequest cancelRequest = cancelRequestHelper(bestAskOrderID, 1, 1002);
    engine.submit(bestAsk);
    engine.submit(nextAsk);

    SubmitResult result = engine.submit(cancelRequest);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1003);
    EXPECT_EQ(result.reason, RejectionReason::None);
    ASSERT_TRUE(snapshot.bestAsk.has_value());
    EXPECT_EQ(*snapshot.bestAsk, nextAskPrice);
}

TEST_F(MatchingEngineCancelTest, Submit_CancelOnlyBuy_LeavesBidSideEmpty) {
    OrderID restingOrderID = 1;
    NewOrderRequest restingBuy = newOrderRequestHelper(restingOrderID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    CancelRequest cancelRequest = cancelRequestHelper(restingOrderID, 1, 1001);
    engine.submit(restingBuy);

    SubmitResult result = engine.submit(cancelRequest);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1002);
    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_FALSE(snapshot.bestBid.has_value());
    EXPECT_EQ(snapshot.bidSummary.totalOrderCount, static_cast<Count>(0));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelOnlySell_LeavesAskSideEmpty) {
    OrderID restingOrderID = 1;
    NewOrderRequest restingSell = newOrderRequestHelper(restingOrderID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    CancelRequest cancelRequest = cancelRequestHelper(restingOrderID, 1, 1001);
    engine.submit(restingSell);

    SubmitResult result = engine.submit(cancelRequest);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1002);
    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_FALSE(snapshot.bestAsk.has_value());
    EXPECT_EQ(snapshot.askSummary.totalOrderCount, static_cast<Count>(0));
}

// =====================================================================
// Partially filled orders
// =====================================================================

TEST_F(MatchingEngineCancelTest, Submit_CancelPartiallyFilledBuy_LandsInCancelledAfterPartialExecution) {
    OrderID partiallyFilledOrderID = 1;
    NewOrderRequest restingBuy = newOrderRequestHelper(partiallyFilledOrderID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest partiallyFillingSell = newOrderRequestHelper(2, 2, 100, 5, Side::Sell, OrderType::Limit, 1001);
    CancelRequest cancelRequest = cancelRequestHelper(partiallyFilledOrderID, 1, 1002);
    OrderPtr partiallyFilledOrder = engine.submit(restingBuy).order;
    engine.submit(partiallyFillingSell);

    SubmitResult result = engine.submit(cancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order, partiallyFilledOrder);
    EXPECT_EQ(partiallyFilledOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(partiallyFilledOrder->getQty(), static_cast<Quantity>(5));
    EXPECT_FALSE(orderBook.doesOrderExist(partiallyFilledOrderID));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelPartiallyFilledSell_LandsInCancelledAfterPartialExecution) {
    OrderID partiallyFilledOrderID = 1;
    NewOrderRequest restingSell = newOrderRequestHelper(partiallyFilledOrderID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest partiallyFillingBuy = newOrderRequestHelper(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    CancelRequest cancelRequest = cancelRequestHelper(partiallyFilledOrderID, 1, 1002);
    OrderPtr partiallyFilledOrder = engine.submit(restingSell).order;
    engine.submit(partiallyFillingBuy);

    SubmitResult result = engine.submit(cancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order, partiallyFilledOrder);
    EXPECT_EQ(partiallyFilledOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(partiallyFilledOrder->getQty(), static_cast<Quantity>(5));
    EXPECT_FALSE(orderBook.doesOrderExist(partiallyFilledOrderID));
}

// =====================================================================
// Cancel count
// =====================================================================

TEST_F(MatchingEngineCancelTest, Submit_CancelOneBuy_CancelCountIsOne) {
    OrderID restingOrderID = 1;
    NewOrderRequest restingBuy = newOrderRequestHelper(restingOrderID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    CancelRequest cancelRequest = cancelRequestHelper(restingOrderID, 1, 1001);
    engine.submit(restingBuy);

    engine.submit(cancelRequest);

    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(1));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelOneSell_CancelCountIsOne) {
    OrderID restingOrderID = 1;
    NewOrderRequest restingSell = newOrderRequestHelper(restingOrderID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    CancelRequest cancelRequest = cancelRequestHelper(restingOrderID, 1, 1001);
    engine.submit(restingSell);

    engine.submit(cancelRequest);

    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(1));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelTwoBuys_CancelCountIsTwo) {
    OrderID firstOrderID = 1;
    OrderID secondOrderID = 2;
    NewOrderRequest firstBuy = newOrderRequestHelper(firstOrderID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondBuy = newOrderRequestHelper(secondOrderID, 1, 99, 10, Side::Buy, OrderType::Limit, 1001);
    CancelRequest firstCancel = cancelRequestHelper(firstOrderID, 1, 1002);
    CancelRequest secondCancel = cancelRequestHelper(secondOrderID, 1, 1003);
    engine.submit(firstBuy);
    engine.submit(secondBuy);

    engine.submit(firstCancel);
    engine.submit(secondCancel);

    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(2));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelTwoSells_CancelCountIsTwo) {
    OrderID firstOrderID = 1;
    OrderID secondOrderID = 2;
    NewOrderRequest firstSell = newOrderRequestHelper(firstOrderID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondSell = newOrderRequestHelper(secondOrderID, 1, 101, 10, Side::Sell, OrderType::Limit, 1001);
    CancelRequest firstCancel = cancelRequestHelper(firstOrderID, 1, 1002);
    CancelRequest secondCancel = cancelRequestHelper(secondOrderID, 1, 1003);
    engine.submit(firstSell);
    engine.submit(secondSell);

    engine.submit(firstCancel);
    engine.submit(secondCancel);

    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(2));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelOneBuyAndOneSell_CancelCountIsTwo) {
    OrderID buyOrderID = 1;
    OrderID sellOrderID = 2;
    NewOrderRequest restingBuy = newOrderRequestHelper(buyOrderID, 1, 99, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest restingSell = newOrderRequestHelper(sellOrderID, 1, 101, 10, Side::Sell, OrderType::Limit, 1001);
    CancelRequest buyCancel = cancelRequestHelper(buyOrderID, 1, 1002);
    CancelRequest sellCancel = cancelRequestHelper(sellOrderID, 1, 1003);
    engine.submit(restingBuy);
    engine.submit(restingSell);

    engine.submit(buyCancel);
    engine.submit(sellCancel);

    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(2));
}
