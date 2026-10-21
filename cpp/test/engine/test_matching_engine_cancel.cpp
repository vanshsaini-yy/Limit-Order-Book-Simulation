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
    CancelRequest zeroTargetCancelRequest = cancelRequestHelper(0, 1, 1000);

    SubmitResult result = engine.submit(zeroTargetCancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(0));
}

// =====================================================================
// Order lookup
// =====================================================================

TEST_F(MatchingEngineCancelTest, Submit_CancelNonExistentOrder_RejectedAsOrderToBeCancelledDoesNotExist) {
    OrderID nonExistentID = 999;
    CancelRequest nonExistentCancelRequest = cancelRequestHelper(nonExistentID, 1, 1000);

    SubmitResult result = engine.submit(nonExistentCancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(0));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelAnotherOwnersOrder_RejectedAsOrderToBeCancelledDoesNotExist) {
    OrderID restingBuyID = 1;
    OwnerID restingOwnerID = 1;
    OwnerID anotherOwnerID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, restingOwnerID, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr restingBuy = engine.submit(restingBuyRequest).order;
    CancelRequest anotherOwnerCancelRequest = cancelRequestHelper(restingBuyID, anotherOwnerID, 1001);

    SubmitResult result = engine.submit(anotherOwnerCancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_EQ(restingBuy->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingBuy->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(restingBuyID));
    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(0));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelAnotherOwnersOrder_IndistinguishableFromNonExistentOrder) {
    OrderID restingBuyID = 1;
    OwnerID restingOwnerID = 1;
    OwnerID anotherOwnerID = 2;
    OrderID nonExistentID = 999;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, restingOwnerID, 100, 10, Side::Buy, OrderType::Limit, 1000);
    CancelRequest wrongOwnerCancelRequest = cancelRequestHelper(restingBuyID, anotherOwnerID, 1001);
    CancelRequest nonExistentCancelRequest = cancelRequestHelper(nonExistentID, anotherOwnerID, 1002);
    engine.submit(restingBuyRequest);

    SubmitResult wrongOwnerResult = engine.submit(wrongOwnerCancelRequest);
    SubmitResult nonExistentResult = engine.submit(nonExistentCancelRequest);

    EXPECT_EQ(wrongOwnerResult.reason, nonExistentResult.reason);
    EXPECT_EQ(wrongOwnerResult.order, nonExistentResult.order);
    EXPECT_EQ(wrongOwnerResult.order, nullptr);
}

TEST_F(MatchingEngineCancelTest, Submit_CancelAlreadyCancelledOrder_RejectedAsOrderToBeCancelledDoesNotExist) {
    OrderID restingBuyID = 1;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    CancelRequest firstCancelRequest = cancelRequestHelper(restingBuyID, 1, 1001);
    CancelRequest secondCancelRequest = cancelRequestHelper(restingBuyID, 1, 1002);
    engine.submit(restingBuyRequest);
    engine.submit(firstCancelRequest);

    SubmitResult result = engine.submit(secondCancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(1));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelFullyFilledOrder_RejectedAsOrderToBeCancelledDoesNotExist) {
    OrderID restingBuyID = 1;
    OrderID fillingSellID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest fillingSellRequest = newOrderRequestHelper(fillingSellID, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    CancelRequest restingBuyCancelRequest = cancelRequestHelper(restingBuyID, 1, 1002);
    OrderPtr restingBuy = engine.submit(restingBuyRequest).order;
    engine.submit(fillingSellRequest);

    SubmitResult result = engine.submit(restingBuyCancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_EQ(restingBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(0));
}

// =====================================================================
// Successful cancel
// =====================================================================

TEST_F(MatchingEngineCancelTest, Submit_CancelRestingBuy_CancelsOrder) {
    OrderID restingBuyID = 1;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    CancelRequest restingBuyCancelRequest = cancelRequestHelper(restingBuyID, 1, 1001);
    OrderPtr restingBuy = engine.submit(restingBuyRequest).order;

    SubmitResult result = engine.submit(restingBuyCancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order, restingBuy);
    EXPECT_EQ(restingBuy->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(restingBuyID));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelRestingSell_CancelsOrder) {
    OrderID restingSellID = 1;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    CancelRequest restingSellCancelRequest = cancelRequestHelper(restingSellID, 1, 1001);
    OrderPtr restingSell = engine.submit(restingSellRequest).order;

    SubmitResult result = engine.submit(restingSellCancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order, restingSell);
    EXPECT_EQ(restingSell->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(restingSellID));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelOneOfSeveralBuysAtSameLevel_OtherBuysStayPending) {
    OrderID firstBuyID = 1;
    OrderID secondBuyID = 2;
    OrderID thirdBuyID = 3;
    NewOrderRequest firstBuyRequest = newOrderRequestHelper(firstBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondBuyRequest = newOrderRequestHelper(secondBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest thirdBuyRequest = newOrderRequestHelper(thirdBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1002);
    CancelRequest secondBuyCancelRequest = cancelRequestHelper(secondBuyID, 1, 1003);
    OrderPtr firstBuy = engine.submit(firstBuyRequest).order;
    OrderPtr secondBuy = engine.submit(secondBuyRequest).order;
    OrderPtr thirdBuy = engine.submit(thirdBuyRequest).order;

    SubmitResult result = engine.submit(secondBuyCancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(secondBuy->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(secondBuyID));
    EXPECT_EQ(firstBuy->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(firstBuyID));
    EXPECT_EQ(thirdBuy->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(thirdBuyID));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelOneOfSeveralSellsAtSameLevel_OtherSellsStayPending) {
    OrderID firstSellID = 1;
    OrderID secondSellID = 2;
    OrderID thirdSellID = 3;
    NewOrderRequest firstSellRequest = newOrderRequestHelper(firstSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondSellRequest = newOrderRequestHelper(secondSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest thirdSellRequest = newOrderRequestHelper(thirdSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1002);
    CancelRequest secondSellCancelRequest = cancelRequestHelper(secondSellID, 1, 1003);
    OrderPtr firstSell = engine.submit(firstSellRequest).order;
    OrderPtr secondSell = engine.submit(secondSellRequest).order;
    OrderPtr thirdSell = engine.submit(thirdSellRequest).order;

    SubmitResult result = engine.submit(secondSellCancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(secondSell->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(secondSellID));
    EXPECT_EQ(firstSell->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(firstSellID));
    EXPECT_EQ(thirdSell->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(thirdSellID));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelBestBid_NextBidBecomesBestBid) {
    OrderID bestBidID = 1;
    OrderID nextBidID = 2;
    PriceTicks nextBidPrice = 99;
    NewOrderRequest bestBidRequest = newOrderRequestHelper(bestBidID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest nextBidRequest = newOrderRequestHelper(nextBidID, 1, nextBidPrice, 10, Side::Buy, OrderType::Limit, 1001);
    CancelRequest bestBidCancelRequest = cancelRequestHelper(bestBidID, 1, 1002);
    engine.submit(bestBidRequest);
    engine.submit(nextBidRequest);

    SubmitResult result = engine.submit(bestBidCancelRequest);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1003);
    EXPECT_EQ(result.reason, RejectionReason::None);
    ASSERT_TRUE(snapshot.bestBid.has_value());
    EXPECT_EQ(*snapshot.bestBid, nextBidPrice);
}

TEST_F(MatchingEngineCancelTest, Submit_CancelBestAsk_NextAskBecomesBestAsk) {
    OrderID bestAskID = 1;
    OrderID nextAskID = 2;
    PriceTicks nextAskPrice = 101;
    NewOrderRequest bestAskRequest = newOrderRequestHelper(bestAskID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest nextAskRequest = newOrderRequestHelper(nextAskID, 1, nextAskPrice, 10, Side::Sell, OrderType::Limit, 1001);
    CancelRequest bestAskCancelRequest = cancelRequestHelper(bestAskID, 1, 1002);
    engine.submit(bestAskRequest);
    engine.submit(nextAskRequest);

    SubmitResult result = engine.submit(bestAskCancelRequest);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1003);
    EXPECT_EQ(result.reason, RejectionReason::None);
    ASSERT_TRUE(snapshot.bestAsk.has_value());
    EXPECT_EQ(*snapshot.bestAsk, nextAskPrice);
}

TEST_F(MatchingEngineCancelTest, Submit_CancelOnlyBuy_LeavesBidSideEmpty) {
    OrderID onlyBuyID = 1;
    NewOrderRequest onlyBuyRequest = newOrderRequestHelper(onlyBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    CancelRequest onlyBuyCancelRequest = cancelRequestHelper(onlyBuyID, 1, 1001);
    engine.submit(onlyBuyRequest);

    SubmitResult result = engine.submit(onlyBuyCancelRequest);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1002);
    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_FALSE(snapshot.bestBid.has_value());
    EXPECT_EQ(snapshot.bidSummary.totalOrderCount, static_cast<Count>(0));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelOnlySell_LeavesAskSideEmpty) {
    OrderID onlySellID = 1;
    NewOrderRequest onlySellRequest = newOrderRequestHelper(onlySellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    CancelRequest onlySellCancelRequest = cancelRequestHelper(onlySellID, 1, 1001);
    engine.submit(onlySellRequest);

    SubmitResult result = engine.submit(onlySellCancelRequest);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1002);
    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_FALSE(snapshot.bestAsk.has_value());
    EXPECT_EQ(snapshot.askSummary.totalOrderCount, static_cast<Count>(0));
}

// =====================================================================
// Partially filled orders
// =====================================================================

TEST_F(MatchingEngineCancelTest, Submit_CancelPartiallyFilledBuy_LandsInCancelledAfterPartialExecution) {
    OrderID partiallyFilledBuyID = 1;
    OrderID partiallyFillingSellID = 2;
    NewOrderRequest partiallyFilledBuyRequest = newOrderRequestHelper(partiallyFilledBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest partiallyFillingSellRequest = newOrderRequestHelper(partiallyFillingSellID, 2, 100, 5, Side::Sell, OrderType::Limit, 1001);
    CancelRequest partiallyFilledBuyCancelRequest = cancelRequestHelper(partiallyFilledBuyID, 1, 1002);
    OrderPtr partiallyFilledBuy = engine.submit(partiallyFilledBuyRequest).order;
    engine.submit(partiallyFillingSellRequest);

    SubmitResult result = engine.submit(partiallyFilledBuyCancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order, partiallyFilledBuy);
    EXPECT_EQ(partiallyFilledBuy->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(partiallyFilledBuy->getQty(), static_cast<Quantity>(5));
    EXPECT_FALSE(orderBook.doesOrderExist(partiallyFilledBuyID));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelPartiallyFilledSell_LandsInCancelledAfterPartialExecution) {
    OrderID partiallyFilledSellID = 1;
    OrderID partiallyFillingBuyID = 2;
    NewOrderRequest partiallyFilledSellRequest = newOrderRequestHelper(partiallyFilledSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest partiallyFillingBuyRequest = newOrderRequestHelper(partiallyFillingBuyID, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    CancelRequest partiallyFilledSellCancelRequest = cancelRequestHelper(partiallyFilledSellID, 1, 1002);
    OrderPtr partiallyFilledSell = engine.submit(partiallyFilledSellRequest).order;
    engine.submit(partiallyFillingBuyRequest);

    SubmitResult result = engine.submit(partiallyFilledSellCancelRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order, partiallyFilledSell);
    EXPECT_EQ(partiallyFilledSell->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(partiallyFilledSell->getQty(), static_cast<Quantity>(5));
    EXPECT_FALSE(orderBook.doesOrderExist(partiallyFilledSellID));
}

// =====================================================================
// Cancel count
// =====================================================================

TEST_F(MatchingEngineCancelTest, Submit_CancelOneBuy_CancelCountIsOne) {
    OrderID restingBuyID = 1;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    CancelRequest restingBuyCancelRequest = cancelRequestHelper(restingBuyID, 1, 1001);
    engine.submit(restingBuyRequest);

    engine.submit(restingBuyCancelRequest);

    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(1));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelOneSell_CancelCountIsOne) {
    OrderID restingSellID = 1;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    CancelRequest restingSellCancelRequest = cancelRequestHelper(restingSellID, 1, 1001);
    engine.submit(restingSellRequest);

    engine.submit(restingSellCancelRequest);

    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(1));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelTwoBuys_CancelCountIsTwo) {
    OrderID firstBuyID = 1;
    OrderID secondBuyID = 2;
    NewOrderRequest firstBuyRequest = newOrderRequestHelper(firstBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondBuyRequest = newOrderRequestHelper(secondBuyID, 1, 99, 10, Side::Buy, OrderType::Limit, 1001);
    CancelRequest firstBuyCancelRequest = cancelRequestHelper(firstBuyID, 1, 1002);
    CancelRequest secondBuyCancelRequest = cancelRequestHelper(secondBuyID, 1, 1003);
    engine.submit(firstBuyRequest);
    engine.submit(secondBuyRequest);

    engine.submit(firstBuyCancelRequest);
    engine.submit(secondBuyCancelRequest);

    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(2));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelTwoSells_CancelCountIsTwo) {
    OrderID firstSellID = 1;
    OrderID secondSellID = 2;
    NewOrderRequest firstSellRequest = newOrderRequestHelper(firstSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondSellRequest = newOrderRequestHelper(secondSellID, 1, 101, 10, Side::Sell, OrderType::Limit, 1001);
    CancelRequest firstSellCancelRequest = cancelRequestHelper(firstSellID, 1, 1002);
    CancelRequest secondSellCancelRequest = cancelRequestHelper(secondSellID, 1, 1003);
    engine.submit(firstSellRequest);
    engine.submit(secondSellRequest);

    engine.submit(firstSellCancelRequest);
    engine.submit(secondSellCancelRequest);

    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(2));
}

TEST_F(MatchingEngineCancelTest, Submit_CancelOneBuyAndOneSell_CancelCountIsTwo) {
    OrderID restingBuyID = 1;
    OrderID restingSellID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 99, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 101, 10, Side::Sell, OrderType::Limit, 1001);
    CancelRequest restingBuyCancelRequest = cancelRequestHelper(restingBuyID, 1, 1002);
    CancelRequest restingSellCancelRequest = cancelRequestHelper(restingSellID, 1, 1003);
    engine.submit(restingBuyRequest);
    engine.submit(restingSellRequest);

    engine.submit(restingBuyCancelRequest);
    engine.submit(restingSellCancelRequest);

    EXPECT_EQ(orderBook.getOrderCancellationCount(), static_cast<Count>(2));
}
