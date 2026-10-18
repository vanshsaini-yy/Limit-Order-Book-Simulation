#include <gtest/gtest.h>
#include "models/new_order_request.hpp"

namespace {
NewOrderRequest makeLimit(
    OrderID orderID = 1,
    PriceTicks priceTicks = 100,
    Quantity qty = 10,
    TimeInForce timeInForce = TimeInForce::GTC,
    bool postOnly = false
) {
    return NewOrderRequest(1, 1, 1622547800, orderID, priceTicks, qty, Side::Buy, OrderType::Limit, timeInForce, postOnly);
}

NewOrderRequest makeMarket(
    OrderID orderID = 1,
    PriceTicks priceTicks = 0,
    Quantity qty = 10,
    TimeInForce timeInForce = TimeInForce::GTC,
    bool postOnly = false
) {
    return NewOrderRequest(1, 1, 1622547800, orderID, priceTicks, qty, Side::Buy, OrderType::Market, timeInForce, postOnly);
}
}

// =====================================================================
// Accessors
// =====================================================================

TEST(NewOrderRequestTest, Accessors_ReturnConstructorValues) {
    NewOrderRequest request(7, 3, 1622547800, 42, 105, 12, Side::Sell, OrderType::Limit, TimeInForce::IOC, false);

    EXPECT_EQ(request.getRequestId(), static_cast<RequestID>(7));
    EXPECT_EQ(request.getRequestType(), RequestType::New);
    EXPECT_EQ(request.getOwnerID(), static_cast<OwnerID>(3));
    EXPECT_EQ(request.getTimestamp(), static_cast<Timestamp>(1622547800));
    EXPECT_EQ(request.getOrderID(), static_cast<OrderID>(42));
    EXPECT_EQ(request.getPriceTicks(), static_cast<PriceTicks>(105));
    EXPECT_EQ(request.getQty(), static_cast<Quantity>(12));
    EXPECT_EQ(request.getSide(), Side::Sell);
    EXPECT_EQ(request.getType(), OrderType::Limit);
    EXPECT_EQ(request.getTimeInForce(), TimeInForce::IOC);
    EXPECT_FALSE(request.isPostOnly());
}

// =====================================================================
// Default values for TIF and post-only flag
// =====================================================================

TEST(NewOrderRequestTest, Constructor_OmittedTimeInForceAndPostOnly_DefaultToGTCAndFalse) {
    NewOrderRequest request(1, 1, 1622547800, 1, 100, 10, Side::Buy, OrderType::Limit);

    EXPECT_EQ(request.getTimeInForce(), TimeInForce::GTC);
    EXPECT_FALSE(request.isPostOnly());
}

// =====================================================================
// Limit order validation
// =====================================================================

TEST(NewOrderRequestTest, Validate_WellFormedLimit_IsAccepted) {
    EXPECT_EQ(makeLimit().validate(), RejectionReason::None);
}

TEST(NewOrderRequestTest, Validate_LimitWithZeroOrderID_IsRejected) {
    EXPECT_EQ(makeLimit(0).validate(), RejectionReason::InvalidLimitOrder);
}

TEST(NewOrderRequestTest, Validate_LimitWithZeroQty_IsRejected) {
    EXPECT_EQ(makeLimit(1, 100, 0).validate(), RejectionReason::InvalidLimitOrder);
}

TEST(NewOrderRequestTest, Validate_LimitWithNegativeQty_IsRejected) {
    EXPECT_EQ(makeLimit(1, 100, -5).validate(), RejectionReason::InvalidLimitOrder);
}

TEST(NewOrderRequestTest, Validate_LimitWithZeroPrice_IsRejected) {
    EXPECT_EQ(makeLimit(1, 0).validate(), RejectionReason::InvalidLimitOrder);
}

TEST(NewOrderRequestTest, Validate_LimitWithNegativePrice_IsRejected) {
    EXPECT_EQ(makeLimit(1, -100).validate(), RejectionReason::InvalidLimitOrder);
}

TEST(NewOrderRequestTest, Validate_LimitIOC_IsAccepted) {
    EXPECT_EQ(makeLimit(1, 100, 10, TimeInForce::IOC).validate(), RejectionReason::None);
}

TEST(NewOrderRequestTest, Validate_LimitFOK_IsAccepted) {
    EXPECT_EQ(makeLimit(1, 100, 10, TimeInForce::FOK).validate(), RejectionReason::None);
}

// =====================================================================
// Post-only order validation
// =====================================================================

TEST(NewOrderRequestTest, Validate_PostOnlyGTCLimit_IsAccepted) {
    EXPECT_EQ(makeLimit(1, 100, 10, TimeInForce::GTC, true).validate(), RejectionReason::None);
}

TEST(NewOrderRequestTest, Validate_PostOnlyIOCLimit_IsRejectedAsInvalidPostOnly) {
    EXPECT_EQ(makeLimit(1, 100, 10, TimeInForce::IOC, true).validate(), RejectionReason::InvalidPostOnlyOrder);
}

TEST(NewOrderRequestTest, Validate_PostOnlyFOKLimit_IsRejectedAsInvalidPostOnly) {
    EXPECT_EQ(makeLimit(1, 100, 10, TimeInForce::FOK, true).validate(), RejectionReason::InvalidPostOnlyOrder);
}

// =====================================================================
// Market orders validation
// =====================================================================

TEST(NewOrderRequestTest, Validate_WellFormedMarket_IsAccepted) {
    EXPECT_EQ(makeMarket().validate(), RejectionReason::None);
}

TEST(NewOrderRequestTest, Validate_MarketWithZeroOrderID_IsRejected) {
    EXPECT_EQ(makeMarket(0).validate(), RejectionReason::InvalidMarketOrder);
}

TEST(NewOrderRequestTest, Validate_MarketWithZeroQty_IsRejected) {
    EXPECT_EQ(makeMarket(1, 0, 0).validate(), RejectionReason::InvalidMarketOrder);
}

TEST(NewOrderRequestTest, Validate_MarketWithNegativeQty_IsRejected) {
    EXPECT_EQ(makeMarket(1, 0, -5).validate(), RejectionReason::InvalidMarketOrder);
}

TEST(NewOrderRequestTest, Validate_MarketWithNonZeroPrice_IsRejected) {
    EXPECT_EQ(makeMarket(1, 100).validate(), RejectionReason::InvalidMarketOrder);
}

TEST(NewOrderRequestTest, Validate_MarketWithPostOnly_IsRejectedAsInvalidMarket) {
    EXPECT_EQ(makeMarket(1, 0, 10, TimeInForce::GTC, true).validate(), RejectionReason::InvalidMarketOrder);
}

TEST(NewOrderRequestTest, Validate_MarketIOC_IsAccepted) {
    EXPECT_EQ(makeMarket(1, 0, 10, TimeInForce::IOC).validate(), RejectionReason::None);
}

TEST(NewOrderRequestTest, Validate_MarketFOK_IsAccepted) {
    EXPECT_EQ(makeMarket(1, 0, 10, TimeInForce::FOK).validate(), RejectionReason::None);
}
