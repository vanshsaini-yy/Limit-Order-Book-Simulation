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
    OrderID orderID = 1;
    NewOrderRequest request = newOrderRequestHelper(orderID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000, TimeInForce::IOC, true);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidPostOnlyOrder);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_FALSE(orderBook.doesOrderExist(orderID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyWithFOK_RejectedAsInvalidPostOnlyOrder) {
    OrderID orderID = 1;
    NewOrderRequest request = newOrderRequestHelper(orderID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000, TimeInForce::FOK, true);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidPostOnlyOrder);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_FALSE(orderBook.doesOrderExist(orderID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyMarketOrder_RejectedAsInvalidMarketOrder) {
    OrderID orderID = 1;
    NewOrderRequest request = newOrderRequestHelper(orderID, 1, 0, 10, Side::Buy, OrderType::Market, 1000, TimeInForce::GTC, true);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_FALSE(orderBook.doesOrderExist(orderID));
}

// =====================================================================
// Post-only limit buy
// =====================================================================

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyAtBestAsk_RejectedAsPostOnlyWouldCross) {
    OrderID restingSellID = 1;
    OrderID postOnlyBuyID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest postOnlyBuyRequest = newOrderRequestHelper(postOnlyBuyID, 2, 100, 10, Side::Buy, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingSellRequest);

    SubmitResult result = engine.submit(postOnlyBuyRequest);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(postOnlyBuyID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyAboveBestAsk_RejectedAsPostOnlyWouldCross) {
    OrderID restingSellID = 1;
    OrderID postOnlyBuyID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest postOnlyBuyRequest = newOrderRequestHelper(postOnlyBuyID, 2, 101, 10, Side::Buy, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingSellRequest);

    SubmitResult result = engine.submit(postOnlyBuyRequest);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(postOnlyBuyID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyBelowBestAsk_RestsAsPending) {
    OrderID restingSellID = 1;
    OrderID postOnlyBuyID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest postOnlyBuyRequest = newOrderRequestHelper(postOnlyBuyID, 2, 99, 10, Side::Buy, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingSellRequest);

    SubmitResult result = engine.submit(postOnlyBuyRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(postOnlyBuyID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyAtBestAskWithMultipleAskLevels_RejectedAsPostOnlyWouldCross) {
    OrderID bestAskID = 1;
    OrderID deeperAskID = 2;
    OrderID postOnlyBuyID = 3;
    NewOrderRequest bestAskRequest = newOrderRequestHelper(bestAskID, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest deeperAskRequest = newOrderRequestHelper(deeperAskID, 1, 101, 5, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest postOnlyBuyRequest = newOrderRequestHelper(postOnlyBuyID, 2, 100, 10, Side::Buy, OrderType::Limit, 1002, TimeInForce::GTC, true);
    engine.submit(bestAskRequest);
    engine.submit(deeperAskRequest);

    SubmitResult result = engine.submit(postOnlyBuyRequest);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(postOnlyBuyID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyOnEmptyBook_RestsAsPending) {
    OrderID postOnlyBuyID = 1;
    NewOrderRequest postOnlyBuyRequest = newOrderRequestHelper(postOnlyBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000, TimeInForce::GTC, true);

    SubmitResult result = engine.submit(postOnlyBuyRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(postOnlyBuyID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyAtBestBid_RestsAtSamePriceLevel) {
    OrderID restingBuyID = 1;
    OrderID postOnlyBuyID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest postOnlyBuyRequest = newOrderRequestHelper(postOnlyBuyID, 2, 100, 10, Side::Buy, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingBuyRequest);

    SubmitResult result = engine.submit(postOnlyBuyRequest);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1002);
    LevelInfo bestBidLevel = snapshot.bidDepths.front();
    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(postOnlyBuyID));
    EXPECT_EQ(bestBidLevel.orderCount, static_cast<Count>(2));
}

// =====================================================================
// Post-only limit sell
// =====================================================================

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellAtBestBid_RejectedAsPostOnlyWouldCross) {
    OrderID restingBuyID = 1;
    OrderID postOnlySellID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest postOnlySellRequest = newOrderRequestHelper(postOnlySellID, 2, 100, 10, Side::Sell, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingBuyRequest);

    SubmitResult result = engine.submit(postOnlySellRequest);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(postOnlySellID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellBelowBestBid_RejectedAsPostOnlyWouldCross) {
    OrderID restingBuyID = 1;
    OrderID postOnlySellID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest postOnlySellRequest = newOrderRequestHelper(postOnlySellID, 2, 99, 10, Side::Sell, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingBuyRequest);

    SubmitResult result = engine.submit(postOnlySellRequest);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(postOnlySellID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellAboveBestBid_RestsAsPending) {
    OrderID restingBuyID = 1;
    OrderID postOnlySellID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest postOnlySellRequest = newOrderRequestHelper(postOnlySellID, 2, 101, 10, Side::Sell, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingBuyRequest);

    SubmitResult result = engine.submit(postOnlySellRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(postOnlySellID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellAtBestBidWithMultipleBidLevels_RejectedAsPostOnlyWouldCross) {
    OrderID bestBidID = 1;
    OrderID deeperBidID = 2;
    OrderID postOnlySellID = 3;
    NewOrderRequest bestBidRequest = newOrderRequestHelper(bestBidID, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest deeperBidRequest = newOrderRequestHelper(deeperBidID, 1, 99, 5, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest postOnlySellRequest = newOrderRequestHelper(postOnlySellID, 2, 100, 10, Side::Sell, OrderType::Limit, 1002, TimeInForce::GTC, true);
    engine.submit(bestBidRequest);
    engine.submit(deeperBidRequest);

    SubmitResult result = engine.submit(postOnlySellRequest);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(postOnlySellID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellOnEmptyBook_RestsAsPending) {
    OrderID postOnlySellID = 1;
    NewOrderRequest postOnlySellRequest = newOrderRequestHelper(postOnlySellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000, TimeInForce::GTC, true);

    SubmitResult result = engine.submit(postOnlySellRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(postOnlySellID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellAtBestAsk_RestsAtSamePriceLevel) {
    OrderID restingSellID = 1;
    OrderID postOnlySellID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest postOnlySellRequest = newOrderRequestHelper(postOnlySellID, 2, 100, 10, Side::Sell, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingSellRequest);

    SubmitResult result = engine.submit(postOnlySellRequest);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1002);
    LevelInfo bestAskLevel = snapshot.askDepths.front();
    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(postOnlySellID));
    EXPECT_EQ(bestAskLevel.orderCount, static_cast<Count>(2));
}

// =====================================================================
// Rejected post-only leaves the book unchanged
// =====================================================================

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyRejected_RestingSellStaysPendingWithFullQty) {
    OrderID restingSellID = 1;
    OrderID postOnlyBuyID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest postOnlyBuyRequest = newOrderRequestHelper(postOnlyBuyID, 2, 100, 10, Side::Buy, OrderType::Limit, 1001, TimeInForce::GTC, true);
    OrderPtr restingSell = engine.submit(restingSellRequest).order;

    engine.submit(postOnlyBuyRequest);

    EXPECT_EQ(restingSell->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSell->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(restingSellID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellRejected_RestingBuyStaysPendingWithFullQty) {
    OrderID restingBuyID = 1;
    OrderID postOnlySellID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest postOnlySellRequest = newOrderRequestHelper(postOnlySellID, 2, 100, 10, Side::Sell, OrderType::Limit, 1001, TimeInForce::GTC, true);
    OrderPtr restingBuy = engine.submit(restingBuyRequest).order;

    engine.submit(postOnlySellRequest);

    EXPECT_EQ(restingBuy->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingBuy->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(restingBuyID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyRejected_NoTradeExecuted) {
    OrderID restingSellID = 1;
    OrderID postOnlyBuyID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest postOnlyBuyRequest = newOrderRequestHelper(postOnlyBuyID, 2, 100, 10, Side::Buy, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingSellRequest);

    engine.submit(postOnlyBuyRequest);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1002);
    EXPECT_EQ(snapshot.tempo.tradeExecutionCount, static_cast<Count>(0));
    EXPECT_EQ(snapshot.tempo.totalVolumeTraded, static_cast<Quantity>(0));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellRejected_NoTradeExecuted) {
    OrderID restingBuyID = 1;
    OrderID postOnlySellID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest postOnlySellRequest = newOrderRequestHelper(postOnlySellID, 2, 100, 10, Side::Sell, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingBuyRequest);

    engine.submit(postOnlySellRequest);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1002);
    EXPECT_EQ(snapshot.tempo.tradeExecutionCount, static_cast<Count>(0));
    EXPECT_EQ(snapshot.tempo.totalVolumeTraded, static_cast<Quantity>(0));
}

// =====================================================================
// Post-only with self-trade prevention
// =====================================================================

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlyBuyFromSameOwnerAsBestAsk_RejectedAsPostOnlyWouldCross) {
    OwnerID sameOwnerID = 1;
    OrderID restingSellID = 1;
    OrderID postOnlyBuyID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, sameOwnerID, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest postOnlyBuyRequest = newOrderRequestHelper(postOnlyBuyID, sameOwnerID, 100, 10, Side::Buy, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingSellRequest);

    SubmitResult result = engine.submit(postOnlyBuyRequest);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(postOnlyBuyID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_PostOnlySellFromSameOwnerAsBestBid_RejectedAsPostOnlyWouldCross) {
    OwnerID sameOwnerID = 1;
    OrderID restingBuyID = 1;
    OrderID postOnlySellID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, sameOwnerID, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest postOnlySellRequest = newOrderRequestHelper(postOnlySellID, sameOwnerID, 100, 10, Side::Sell, OrderType::Limit, 1001, TimeInForce::GTC, true);
    engine.submit(restingBuyRequest);

    SubmitResult result = engine.submit(postOnlySellRequest);

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(postOnlySellID));
}

// =====================================================================
// Post-only as maker
// =====================================================================

TEST_F(MatchingEnginePostOnlyTest, Submit_IncomingSellAgainstRestingPostOnlyBuy_PostOnlyOrderExecuted) {
    OrderID postOnlyBuyID = 1;
    OrderID incomingSellID = 2;
    NewOrderRequest postOnlyBuyRequest = newOrderRequestHelper(postOnlyBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000, TimeInForce::GTC, true);
    NewOrderRequest incomingSellRequest = newOrderRequestHelper(incomingSellID, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    OrderPtr postOnlyBuy = engine.submit(postOnlyBuyRequest).order;

    SubmitResult result = engine.submit(incomingSellRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(postOnlyBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(postOnlyBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(postOnlyBuyID));
}

TEST_F(MatchingEnginePostOnlyTest, Submit_IncomingBuyAgainstRestingPostOnlySell_PostOnlyOrderExecuted) {
    OrderID postOnlySellID = 1;
    OrderID incomingBuyID = 2;
    NewOrderRequest postOnlySellRequest = newOrderRequestHelper(postOnlySellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000, TimeInForce::GTC, true);
    NewOrderRequest incomingBuyRequest = newOrderRequestHelper(incomingBuyID, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    OrderPtr postOnlySell = engine.submit(postOnlySellRequest).order;

    SubmitResult result = engine.submit(incomingBuyRequest);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(postOnlySell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(postOnlySell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(postOnlySellID));
}
