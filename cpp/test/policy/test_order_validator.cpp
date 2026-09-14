#include <gtest/gtest.h>
#include "policy/order_validator.hpp"

OrderPtr makeLimitOrder(
    OrderID orderID = 1,
    PriceTicks priceTicks = 100,
    Quantity qty = 10,
    Side side = Side::Buy,
    TimeInForce tif = TimeInForce::GTC,
    bool postOnly = false
) {
    constexpr OwnerID kOwnerID = 1;
    constexpr Timestamp kTimestamp = 1622547800;

    return std::make_shared<Order>(
        orderID, kOwnerID, priceTicks, qty, side, OrderType::Limit, kTimestamp, tif, postOnly
    );
}

OrderPtr makeMarketOrder(
    OrderID orderID = 1,
    PriceTicks priceTicks = 0,
    Quantity qty = 10,
    Side side = Side::Buy,
    bool postOnly = false
) {
    constexpr OwnerID kOwnerID = 1;
    constexpr Timestamp kTimestamp = 1622547800;

    return std::make_shared<Order>(
        orderID, kOwnerID, priceTicks, qty, side, OrderType::Market, kTimestamp,
        TimeInForce::GTC, postOnly
    );
}

// =====================================================================
// validateLimitOrder
// =====================================================================

TEST(OrderValidatorTest, ValidateLimitOrder_WellFormed_IsAccepted) {
    EXPECT_EQ(OrderValidator::validateLimitOrder(*makeLimitOrder()),
              RejectionReason::None);
}

TEST(OrderValidatorTest, ValidateLimitOrder_NonPositivePrice_IsRejected) {
    EXPECT_EQ(OrderValidator::validateLimitOrder(*makeLimitOrder(1, 0)),
              RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(OrderValidator::validateLimitOrder(*makeLimitOrder(1, -1)),
              RejectionReason::InvalidLimitOrder);
}

TEST(OrderValidatorTest, ValidateLimitOrder_NonPositiveQty_IsRejected) {
    EXPECT_EQ(OrderValidator::validateLimitOrder(*makeLimitOrder(1, 100, 0)),
              RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(OrderValidator::validateLimitOrder(*makeLimitOrder(1, 100, -1)),
              RejectionReason::InvalidLimitOrder);
}

TEST(OrderValidatorTest, ValidateLimitOrder_ZeroOrderID_IsRejected) {
    EXPECT_EQ(OrderValidator::validateLimitOrder(*makeLimitOrder(0)),
              RejectionReason::InvalidLimitOrder);
}

TEST(OrderValidatorTest, ValidateLimitOrder_PartiallyExecuted_RejectedWhenPartialExecutionNotAllowed) {
    OrderPtr order = makeLimitOrder();
    order->setStatus(OrderStatus::PartiallyExecuted);

    EXPECT_EQ(OrderValidator::validateLimitOrder(*order, false),
              RejectionReason::InvalidLimitOrder);
}

TEST(OrderValidatorTest, ValidateLimitOrder_PartiallyExecuted_AcceptedWhenPartialExecutionAllowed) {
    OrderPtr order = makeLimitOrder();
    order->setStatus(OrderStatus::PartiallyExecuted);

    EXPECT_EQ(OrderValidator::validateLimitOrder(*order, true),
              RejectionReason::None);
}

TEST(OrderValidatorTest, ValidateLimitOrder_PostOnlyWithNonGTC_IsRejected) {
    OrderPtr order = makeLimitOrder(1, 100, 10, Side::Buy, TimeInForce::IOC, true);

    EXPECT_EQ(OrderValidator::validateLimitOrder(*order),
              RejectionReason::InvalidPostOnlyOrder);
}

TEST(OrderValidatorTest, ValidateLimitOrder_PostOnlyWithGTC_IsAccepted) {
    OrderPtr order = makeLimitOrder(1, 100, 10, Side::Buy, TimeInForce::GTC, true);

    EXPECT_EQ(OrderValidator::validateLimitOrder(*order),
              RejectionReason::None);
}

// =====================================================================
// validateMarketOrder
// =====================================================================

TEST(OrderValidatorTest, ValidateMarketOrder_WellFormed_IsAccepted) {
    EXPECT_EQ(OrderValidator::validateMarketOrder(*makeMarketOrder()),
              RejectionReason::None);
}

TEST(OrderValidatorTest, ValidateMarketOrder_NonZeroPrice_IsRejected) {
    EXPECT_EQ(OrderValidator::validateMarketOrder(*makeMarketOrder(1, 100)),
              RejectionReason::InvalidMarketOrder);
}

TEST(OrderValidatorTest, ValidateMarketOrder_NonPositiveQty_IsRejected) {
    EXPECT_EQ(OrderValidator::validateMarketOrder(*makeMarketOrder(1, 0, 0)),
              RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(OrderValidator::validateMarketOrder(*makeMarketOrder(1, 0, -1)),
              RejectionReason::InvalidMarketOrder);
}

TEST(OrderValidatorTest, ValidateMarketOrder_NonPendingStatus_IsRejected) {
    OrderPtr order = makeMarketOrder();
    order->setStatus(OrderStatus::PartiallyExecuted);

    EXPECT_EQ(OrderValidator::validateMarketOrder(*order),
              RejectionReason::InvalidMarketOrder);
}

TEST(OrderValidatorTest, ValidateMarketOrder_ZeroOrderID_IsRejected) {
    EXPECT_EQ(OrderValidator::validateMarketOrder(*makeMarketOrder(0)),
              RejectionReason::InvalidMarketOrder);
}

TEST(OrderValidatorTest, ValidateMarketOrder_PostOnly_IsRejected) {
    OrderPtr order = makeMarketOrder(1, 0, 10, Side::Buy, true);

    EXPECT_EQ(OrderValidator::validateMarketOrder(*order),
              RejectionReason::InvalidMarketOrder);
}

// =====================================================================
// validateBeforeAddingOrRemoving
// =====================================================================

TEST(OrderValidatorTest, ValidateBeforeAddingOrRemoving_NullOrder_IsRejected) {
    EXPECT_EQ(OrderValidator::validateBeforeAddingOrRemoving(nullptr),
              RejectionReason::OrderBookInvariantViolation);
}

TEST(OrderValidatorTest, ValidateBeforeAddingOrRemoving_PendingLimitOrder_IsAccepted) {
    EXPECT_EQ(OrderValidator::validateBeforeAddingOrRemoving(makeLimitOrder()),
              RejectionReason::None);
}

TEST(OrderValidatorTest, ValidateBeforeAddingOrRemoving_PartiallyExecutedLimitOrder_IsAccepted) {
    OrderPtr order = makeLimitOrder();
    order->setStatus(OrderStatus::PartiallyExecuted);

    EXPECT_EQ(OrderValidator::validateBeforeAddingOrRemoving(order),
              RejectionReason::None);
}

TEST(OrderValidatorTest, ValidateBeforeAddingOrRemoving_InvalidLimitOrder_IsRejected) {
    EXPECT_EQ(OrderValidator::validateBeforeAddingOrRemoving(makeLimitOrder(1, 0)),
              RejectionReason::OrderBookInvariantViolation);
}

// =====================================================================
// validateBeforeMatching
// =====================================================================

TEST(OrderValidatorTest, ValidateBeforeMatching_NullOrder_IsRejected) {
    EXPECT_EQ(OrderValidator::validateBeforeMatching(nullptr),
              RejectionReason::NullOrder);
}

TEST(OrderValidatorTest, ValidateBeforeMatching_ValidLimitOrder_DelegatesToValidateLimitOrder) {
    EXPECT_EQ(OrderValidator::validateBeforeMatching(makeLimitOrder()),
              RejectionReason::None);
}

TEST(OrderValidatorTest, ValidateBeforeMatching_InvalidLimitOrder_DelegatesToValidateLimitOrder) {
    EXPECT_EQ(OrderValidator::validateBeforeMatching(makeLimitOrder(1, 0)),
              RejectionReason::InvalidLimitOrder);
}

TEST(OrderValidatorTest, ValidateBeforeMatching_ValidMarketOrder_DelegatesToValidateMarketOrder) {
    EXPECT_EQ(OrderValidator::validateBeforeMatching(makeMarketOrder()),
              RejectionReason::None);
}

TEST(OrderValidatorTest, ValidateBeforeMatching_InvalidMarketOrder_DelegatesToValidateMarketOrder) {
    EXPECT_EQ(OrderValidator::validateBeforeMatching(makeMarketOrder(1, 0, 0)),
              RejectionReason::InvalidMarketOrder);
}
