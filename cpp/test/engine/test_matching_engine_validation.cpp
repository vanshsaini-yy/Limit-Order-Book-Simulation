#include <gtest/gtest.h>
#include <memory>
#include "engine/matching_engine.hpp"
#include "support/new_order.hpp"

class MatchingEngineValidationTest : public ::testing::Test {
protected:
    CancelBothSTP stpPolicy;
    LimitOrderBook orderBook;
    MatchingEngine engine{&orderBook, &stpPolicy};
};

// =====================================================================
// Limit order field validation
// =====================================================================

TEST_F(MatchingEngineValidationTest, Submit_LimitOrderWithZeroPrice_RejectedAsInvalidLimitOrder) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, 0, 10, Side::Buy, OrderType::Limit, 1000);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(result.order, nullptr);
}

TEST_F(MatchingEngineValidationTest, Submit_LimitOrderWithNegativePrice_RejectedAsInvalidLimitOrder) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, -100, 10, Side::Buy, OrderType::Limit, 1000);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(result.order, nullptr);
}

TEST_F(MatchingEngineValidationTest, Submit_LimitOrderWithZeroQty_RejectedAsInvalidLimitOrder) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, 100, 0, Side::Buy, OrderType::Limit, 1000);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(result.order, nullptr);
}

TEST_F(MatchingEngineValidationTest, Submit_LimitOrderWithNegativeQty_RejectedAsInvalidLimitOrder) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, 100, -10, Side::Buy, OrderType::Limit, 1000);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(result.order, nullptr);
}

TEST_F(MatchingEngineValidationTest, Submit_LimitOrderWithZeroOrderID_RejectedAsInvalidLimitOrder) {
    NewOrderRequest request = newOrderRequestHelper(0, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(result.order, nullptr);
}

// =====================================================================
// Market order field validation
// =====================================================================

TEST_F(MatchingEngineValidationTest, Submit_MarketOrderWithNonZeroPrice_RejectedAsInvalidMarketOrder) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, 100, 10, Side::Sell, OrderType::Market, 1000);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(result.order, nullptr);
}

TEST_F(MatchingEngineValidationTest, Submit_MarketOrderWithZeroQty_RejectedAsInvalidMarketOrder) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, 0, 0, Side::Sell, OrderType::Market, 1000);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(result.order, nullptr);
}

TEST_F(MatchingEngineValidationTest, Submit_MarketOrderWithNegativeQty_RejectedAsInvalidMarketOrder) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, 0, -10, Side::Sell, OrderType::Market, 1000);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(result.order, nullptr);
}

TEST_F(MatchingEngineValidationTest, Submit_MarketOrderWithZeroOrderID_RejectedAsInvalidMarketOrder) {
    NewOrderRequest request = newOrderRequestHelper(0, 1, 0, 10, Side::Sell, OrderType::Market, 1000);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(result.order, nullptr);
}

// =====================================================================
// Rejected requests leave the book untouched
// =====================================================================

TEST_F(MatchingEngineValidationTest, Submit_InvalidLimitOrder_BookStaysEmpty) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, 0, 10, Side::Buy, OrderType::Limit, 1000);

    engine.submit(request);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1001);
    EXPECT_EQ(snapshot.bidSummary.totalOrderCount, 0u);
    EXPECT_EQ(snapshot.askSummary.totalOrderCount, 0u);
}

TEST_F(MatchingEngineValidationTest, Submit_InvalidMarketOrder_BookStaysEmpty) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, 100, 10, Side::Sell, OrderType::Market, 1000);

    engine.submit(request);

    MarketStructureSnapshot snapshot = orderBook.snapshot(1001);
    EXPECT_EQ(snapshot.bidSummary.totalOrderCount, 0u);
    EXPECT_EQ(snapshot.askSummary.totalOrderCount, 0u);
}

// =====================================================================
// Duplicate order ID
// =====================================================================

TEST_F(MatchingEngineValidationTest, Submit_LimitOrderWithRestingOrderID_RejectedAsDuplicateOrderID) {
    NewOrderRequest restingRequest = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest duplicateRequest = newOrderRequestHelper(1, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    engine.submit(restingRequest);

    SubmitResult result = engine.submit(duplicateRequest);

    EXPECT_EQ(result.reason, RejectionReason::DuplicateOrderID);
    EXPECT_EQ(result.order, nullptr);
}

TEST_F(MatchingEngineValidationTest, Submit_MarketOrderWithRestingOrderID_RejectedAsDuplicateOrderID) {
    NewOrderRequest restingRequest = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest duplicateRequest = newOrderRequestHelper(1, 2, 0, 20, Side::Sell, OrderType::Market, 1001);
    engine.submit(restingRequest);

    SubmitResult result = engine.submit(duplicateRequest);

    EXPECT_EQ(result.reason, RejectionReason::DuplicateOrderID);
    EXPECT_EQ(result.order, nullptr);
}

TEST_F(MatchingEngineValidationTest, Submit_DuplicateOrderID_RestingOrderStaysPending) {
    NewOrderRequest restingRequest = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest duplicateRequest = newOrderRequestHelper(1, 2, 0, 20, Side::Sell, OrderType::Market, 1001);
    OrderPtr restingOrder = engine.submit(restingRequest).order;

    engine.submit(duplicateRequest);

    EXPECT_EQ(restingOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingOrder->getQty(), static_cast<Quantity>(10));
}

// =====================================================================
// Boundary values are accepted
// =====================================================================

TEST_F(MatchingEngineValidationTest, Submit_LimitOrderWithMinimumPrice_AcceptedAndRests) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, 1, 10, Side::Buy, OrderType::Limit, 1000);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
}

TEST_F(MatchingEngineValidationTest, Submit_LimitOrderWithMinimumQty_AcceptedAndRests) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, 100, 1, Side::Buy, OrderType::Limit, 1000);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
}

TEST_F(MatchingEngineValidationTest, Submit_MarketOrderWithMinimumQty_AcceptedAndDiscardedOnEmptyBook) {
    NewOrderRequest request = newOrderRequestHelper(1, 1, 0, 1, Side::Sell, OrderType::Market, 1000);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
}
