#include <gtest/gtest.h>
#include "engine/matching_engine.hpp"
#include "support/new_order.hpp"

class MatchingEnginePostOnlyTest : public ::testing::Test {
protected:
    CancelBothSTP stpPolicy;
    LimitOrderBook orderBook;
    MatchingEngine engine{&orderBook, &stpPolicy};
};

// =====================================================================
// Post-only validation
// =====================================================================

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyWithIOC_RejectedAsInvalidPostOnlyOrder) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000, TimeInForce::IOC, true);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidPostOnlyOrder);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_FALSE(orderBook.doesOrderExist(1));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyWithFOK_RejectedAsInvalidPostOnlyOrder) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000, TimeInForce::FOK, true);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidPostOnlyOrder);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_FALSE(orderBook.doesOrderExist(1));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyMarketOrder_RejectedAsInvalidMarketOrder) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, 0, 10, Side::Buy, OrderType::Market, 1000, TimeInForce::GTC, true);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_FALSE(orderBook.doesOrderExist(1));
}

// =====================================================================
// Post-only limit buy
// =====================================================================

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyAtBestAsk_RejectedAsPostOnlyWouldCross) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest postOnlyBuy = newOrderRequestHelper(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingSell);

    SubmitResult result = engine.submit(postOnlyBuy);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(2));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyAboveBestAsk_RejectedAsPostOnlyWouldCross) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest postOnlyBuy = newOrderRequestHelper(2, 2, 101, 10, Side::Buy, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingSell);

    SubmitResult result = engine.submit(postOnlyBuy);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(2));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyBelowBestAsk_RestsAsPending) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest postOnlyBuy = newOrderRequestHelper(2, 2, 99, 10, Side::Buy, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingSell);

    SubmitResult result = engine.submit(postOnlyBuy);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(2));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyAtBestAskWithMultipleAskLevels_RejectedAsPostOnlyWouldCross) {
    NewOrderRequest bestAsk = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest deeperAsk = newOrderRequestHelper(2, 1, 101, 5, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest postOnlyBuy = newOrderRequestHelper(3, 2, 100, 10, Side::Buy, OrderType::Limit, 1002, TimeInForce::GTC, true);
    engine.submit(bestAsk);
    engine.submit(deeperAsk);

    SubmitResult result = engine.submit(postOnlyBuy);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyOnEmptyBook_RestsAsPending) {
    NewOrderRequest postOnlyBuy = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000, TimeInForce::GTC, true);

    SubmitResult result = engine.submit(postOnlyBuy);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(1));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyAtBestBid_RestsAtSamePriceLevel) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest postOnlyBuy = newOrderRequestHelper(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingBuy);

    SubmitResult result = engine.submit(postOnlyBuy);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1002);
    LevelInfo bestBidLevel = snapshot.bidDepths.front();
    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(2));
    EXPECT_EQ(bestBidLevel.orderCount, static_cast<Count>(2));
}

// =====================================================================
// Post-only limit sell
// =====================================================================

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellAtBestBid_RejectedAsPostOnlyWouldCross) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest postOnlySell = newOrderRequestHelper(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingBuy);

    SubmitResult result = engine.submit(postOnlySell);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(2));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellBelowBestBid_RejectedAsPostOnlyWouldCross) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest postOnlySell = newOrderRequestHelper(2, 2, 99, 10, Side::Sell, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingBuy);

    SubmitResult result = engine.submit(postOnlySell);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(2));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellAboveBestBid_RestsAsPending) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest postOnlySell = newOrderRequestHelper(2, 2, 101, 10, Side::Sell, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingBuy);

    SubmitResult result = engine.submit(postOnlySell);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(2));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellAtBestBidWithMultipleBidLevels_RejectedAsPostOnlyWouldCross) {
    NewOrderRequest bestBid = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest deeperBid = newOrderRequestHelper(2, 1, 99, 5, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest postOnlySell = newOrderRequestHelper(3, 2, 100, 10, Side::Sell, OrderType::Limit, 1002, TimeInForce::GTC, true);
    engine.submit(bestBid);
    engine.submit(deeperBid);

    SubmitResult result = engine.submit(postOnlySell);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellOnEmptyBook_RestsAsPending) {
    NewOrderRequest postOnlySell = newOrderRequestHelper(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000, TimeInForce::GTC, true);

    SubmitResult result = engine.submit(postOnlySell);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(1));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellAtBestAsk_RestsAtSamePriceLevel) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest postOnlySell = newOrderRequestHelper(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingSell);

    SubmitResult result = engine.submit(postOnlySell);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1002);
    LevelInfo bestAskLevel = snapshot.askDepths.front();
    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(2));
    EXPECT_EQ(bestAskLevel.orderCount, static_cast<Count>(2));
}

// =====================================================================
// Rejected post-only leaves the book unchanged
// =====================================================================

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyRejected_RestingSellStaysPendingWithFullQty) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest postOnlyBuy = newOrderRequestHelper(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001, TimeInForce::GTC, true);
    OrderPtr restingSellOrder = engine.submit(restingSell).order;

    engine.submit(postOnlyBuy);

    EXPECT_EQ(restingSellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSellOrder->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(1));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellRejected_RestingBuyStaysPendingWithFullQty) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest postOnlySell = newOrderRequestHelper(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001, TimeInForce::GTC, true);
    OrderPtr restingBuyOrder = engine.submit(restingBuy).order;

    engine.submit(postOnlySell);

    EXPECT_EQ(restingBuyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingBuyOrder->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(1));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyRejected_NoTradeExecuted) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest postOnlyBuy = newOrderRequestHelper(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingSell);

    engine.submit(postOnlyBuy);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1002);
    EXPECT_EQ(snapshot.tempo.tradeExecutionCount, static_cast<Count>(0));
    EXPECT_EQ(snapshot.tempo.totalVolumeTraded, static_cast<Quantity>(0));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellRejected_NoTradeExecuted) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest postOnlySell = newOrderRequestHelper(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingBuy);

    engine.submit(postOnlySell);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1002);
    EXPECT_EQ(snapshot.tempo.tradeExecutionCount, static_cast<Count>(0));
    EXPECT_EQ(snapshot.tempo.totalVolumeTraded, static_cast<Quantity>(0));
}

// =====================================================================
// Post-only with self-trade prevention
// =====================================================================

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyFromSameOwnerAsBestAsk_RejectedAsPostOnlyWouldCross) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest postOnlyBuy = newOrderRequestHelper(2, 1, 100, 10, Side::Buy, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingSell);

    SubmitResult result = engine.submit(postOnlyBuy);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(2));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellFromSameOwnerAsBestBid_RejectedAsPostOnlyWouldCross) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest postOnlySell = newOrderRequestHelper(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingBuy);

    SubmitResult result = engine.submit(postOnlySell);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(2));
}

// =====================================================================
// Post-only as maker
// =====================================================================

TEST_F(MatchingEnginePostOnlyTest, Submit_IncomingSellAgainstRestingPostOnlyBuy_PostOnlyOrderExecuted) {
    NewOrderRequest postOnlyBuy = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000, TimeInForce::GTC, true);
    NewOrderRequest incomingSell = newOrderRequestHelper(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    OrderPtr restingPostOnlyOrder = engine.submit(postOnlyBuy).order;

    SubmitResult result = engine.submit(incomingSell);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(restingPostOnlyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingPostOnlyOrder->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(1));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_IncomingBuyAgainstRestingPostOnlySell_PostOnlyOrderExecuted) {
    NewOrderRequest postOnlySell = newOrderRequestHelper(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000, TimeInForce::GTC, true);
    NewOrderRequest incomingBuy = newOrderRequestHelper(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    OrderPtr restingPostOnlyOrder = engine.submit(postOnlySell).order;

    SubmitResult result = engine.submit(incomingBuy);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(restingPostOnlyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingPostOnlyOrder->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(1));
}
