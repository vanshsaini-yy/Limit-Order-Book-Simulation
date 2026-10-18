#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "engine/matching_engine.hpp"
#include "models/new_order_request.hpp"

class MatchingEngineSubmitNewTest : public ::testing::Test {
protected:
    CancelBothSTP stpPolicy;
    LimitOrderBook orderBook;
    std::optional<MatchingEngine> engine;

    void makeEngine(std::optional<PriceTicks> deviation = std::nullopt) {
        engine.emplace(&orderBook, &stpPolicy, nullptr, nullptr, deviation);
    }

    NewOrderRequest limit(
        OrderID orderID,
        OwnerID ownerID,
        PriceTicks priceTicks,
        Quantity qty,
        Side side,
        Timestamp timestamp = 1000,
        TimeInForce timeInForce = TimeInForce::GTC,
        bool postOnly = false
    ) {
        return NewOrderRequest(orderID, ownerID, timestamp, orderID, priceTicks, qty, side, OrderType::Limit, timeInForce, postOnly);
    }

    NewOrderRequest market(
        OrderID orderID,
        OwnerID ownerID,
        Quantity qty,
        Side side,
        Timestamp timestamp = 1000
    ) {
        return NewOrderRequest(orderID, ownerID, timestamp, orderID, 0, qty, side, OrderType::Market);
    }
};

// =====================================================================
// Rejections before an order exists — no order is returned
// =====================================================================

TEST_F(MatchingEngineSubmitNewTest, Submit_ZeroQty_ReturnsNoOrder) {
    makeEngine();

    SubmitResult result = engine->submit(limit(1, 1, 100, 0, Side::Buy));

    EXPECT_EQ(result.reason, RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_FALSE(orderBook.doesOrderExist(1));
}

TEST_F(MatchingEngineSubmitNewTest, Submit_ZeroOrderID_ReturnsNoOrder) {
    makeEngine();

    SubmitResult result = engine->submit(limit(0, 1, 100, 10, Side::Buy));

    EXPECT_EQ(result.reason, RejectionReason::InvalidLimitOrder);
    EXPECT_EQ(result.order, nullptr);
}

TEST_F(MatchingEngineSubmitNewTest, Submit_MarketWithPrice_ReturnsNoOrder) {
    makeEngine();
    NewOrderRequest request(1, 1, 1000, 1, 100, 10, Side::Buy, OrderType::Market);

    SubmitResult result = engine->submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidMarketOrder);
    EXPECT_EQ(result.order, nullptr);
}

TEST_F(MatchingEngineSubmitNewTest, Submit_PostOnlyIOC_ReturnsNoOrder) {
    makeEngine();

    SubmitResult result = engine->submit(limit(1, 1, 100, 10, Side::Buy, 1000, TimeInForce::IOC, true));

    EXPECT_EQ(result.reason, RejectionReason::InvalidPostOnlyOrder);
    EXPECT_EQ(result.order, nullptr);
}

TEST_F(MatchingEngineSubmitNewTest, Submit_PostOnlyFOK_ReturnsNoOrder) {
    makeEngine();

    SubmitResult result = engine->submit(limit(1, 1, 100, 10, Side::Buy, 1000, TimeInForce::FOK, true));

    EXPECT_EQ(result.reason, RejectionReason::InvalidPostOnlyOrder);
    EXPECT_EQ(result.order, nullptr);
}

TEST_F(MatchingEngineSubmitNewTest, Submit_DuplicateOrderID_ReturnsNoOrderAndLeavesRestingOrderUntouched) {
    makeEngine();
    SubmitResult first = engine->submit(limit(1, 1, 100, 10, Side::Buy));

    SubmitResult duplicate = engine->submit(limit(1, 2, 105, 3, Side::Buy));

    EXPECT_EQ(duplicate.reason, RejectionReason::DuplicateOrderID);
    EXPECT_EQ(duplicate.order, nullptr);
    EXPECT_EQ(orderBook.getOrder(1), first.order);
    EXPECT_EQ(first.order->getPriceTicks(), static_cast<PriceTicks>(100));
    EXPECT_EQ(first.order->getQty(), static_cast<Quantity>(10));
    EXPECT_EQ(first.order->getStatus(), OrderStatus::Pending);
}

// =====================================================================
// Rejections after the order is built — the cancelled order is returned
// =====================================================================

TEST_F(MatchingEngineSubmitNewTest, Submit_PostOnlyWouldCross_ReturnsCancelledOrder) {
    makeEngine();
    engine->submit(limit(1, 1, 100, 5, Side::Sell));

    SubmitResult result = engine->submit(limit(2, 2, 100, 10, Side::Buy, 1001, TimeInForce::GTC, true));

    EXPECT_EQ(result.reason, RejectionReason::PostOnlyWouldCross);
    ASSERT_NE(result.order, nullptr);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(2));
}

TEST_F(MatchingEngineSubmitNewTest, Submit_FOKInsufficientLiquidity_ReturnsCancelledOrderAndLeavesBookUntouched) {
    makeEngine();
    SubmitResult resting = engine->submit(limit(1, 1, 100, 5, Side::Sell));

    SubmitResult result = engine->submit(limit(2, 2, 100, 10, Side::Buy, 1001, TimeInForce::FOK));

    EXPECT_EQ(result.reason, RejectionReason::FOKInsufficientLiquidity);
    ASSERT_NE(result.order, nullptr);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(resting.order->getQty(), static_cast<Quantity>(5));
    EXPECT_EQ(orderBook.getTradeExecutionCount(), 0u);
}

TEST_F(MatchingEngineSubmitNewTest, Submit_PriceCollarViolation_ReturnsCancelledOrder) {
    makeEngine(5);
    engine->submit(limit(1, 1, 100, 10, Side::Buy));
    engine->submit(limit(2, 2, 110, 10, Side::Sell));

    SubmitResult result = engine->submit(limit(3, 3, 200, 10, Side::Buy, 1002));

    EXPECT_EQ(result.reason, RejectionReason::PriceCollarViolation);
    ASSERT_NE(result.order, nullptr);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEngineSubmitNewTest, Submit_SelfTradeWithCancelBoth_ReturnsCancelledOrder) {
    makeEngine();
    engine->submit(limit(1, 1, 100, 5, Side::Sell));

    SubmitResult result = engine->submit(limit(2, 1, 100, 5, Side::Buy, 1001));

    EXPECT_EQ(result.reason, RejectionReason::SelfTradePrevention);
    ASSERT_NE(result.order, nullptr);
    EXPECT_TRUE(result.order->isCancelled());
    EXPECT_FALSE(orderBook.doesOrderExist(1));
    EXPECT_FALSE(orderBook.doesOrderExist(2));
}

// =====================================================================
// Accepted orders
// =====================================================================

TEST_F(MatchingEngineSubmitNewTest, Submit_NonCrossingLimit_RestsAndReturnsTheRestingOrder) {
    makeEngine();

    SubmitResult result = engine->submit(limit(1, 1, 100, 10, Side::Buy));

    EXPECT_EQ(result.reason, RejectionReason::None);
    ASSERT_NE(result.order, nullptr);
    EXPECT_EQ(result.order, orderBook.getOrder(1));
    EXPECT_EQ(result.order->getStatus(), OrderStatus::Pending);
}

TEST_F(MatchingEngineSubmitNewTest, Submit_ReturnedOrder_CarriesEveryRequestField) {
    makeEngine();
    NewOrderRequest request(9, 4, 1622547800, 17, 105, 12, Side::Sell, OrderType::Limit, TimeInForce::GTC, true);

    SubmitResult result = engine->submit(request);

    ASSERT_NE(result.order, nullptr);
    EXPECT_EQ(result.order->getOrderID(), static_cast<OrderID>(17));
    EXPECT_EQ(result.order->getOwnerID(), static_cast<OwnerID>(4));
    EXPECT_EQ(result.order->getPriceTicks(), static_cast<PriceTicks>(105));
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(12));
    EXPECT_EQ(result.order->getOriginalQty(), static_cast<Quantity>(12));
    EXPECT_EQ(result.order->getSide(), Side::Sell);
    EXPECT_EQ(result.order->getType(), OrderType::Limit);
    EXPECT_EQ(result.order->getTimestamp(), static_cast<Timestamp>(1622547800));
    EXPECT_EQ(result.order->getTimeInForce(), TimeInForce::GTC);
    EXPECT_TRUE(result.order->isPostOnly());
}

TEST_F(MatchingEngineSubmitNewTest, Submit_FullyFilledIncoming_ReturnsExecutedOrderOutsideTheBook) {
    makeEngine();
    SubmitResult resting = engine->submit(limit(1, 1, 100, 10, Side::Sell));

    SubmitResult result = engine->submit(limit(2, 2, 100, 10, Side::Buy, 1001));

    EXPECT_EQ(result.reason, RejectionReason::None);
    ASSERT_NE(result.order, nullptr);
    EXPECT_TRUE(result.order->isExecuted());
    EXPECT_FALSE(orderBook.doesOrderExist(2));
    EXPECT_TRUE(resting.order->isExecuted());
}

TEST_F(MatchingEngineSubmitNewTest, Submit_PartiallyFilledLimit_ReturnsPartiallyExecutedOrderRestingTheRemainder) {
    makeEngine();
    engine->submit(limit(1, 1, 100, 5, Side::Sell));

    SubmitResult result = engine->submit(limit(2, 2, 100, 8, Side::Buy, 1001));

    EXPECT_EQ(result.reason, RejectionReason::None);
    ASSERT_NE(result.order, nullptr);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(result.order->getQty(), static_cast<Quantity>(3));
    EXPECT_EQ(result.order->getFilledQty(), static_cast<Quantity>(5));
    EXPECT_EQ(result.order, orderBook.getOrder(2));
}

TEST_F(MatchingEngineSubmitNewTest, Submit_IOCRemainder_IsDiscardedAndOrderIsReturned) {
    makeEngine();
    engine->submit(limit(1, 1, 100, 3, Side::Sell));

    SubmitResult result = engine->submit(limit(2, 2, 100, 5, Side::Buy, 1001, TimeInForce::IOC));

    EXPECT_EQ(result.reason, RejectionReason::None);
    ASSERT_NE(result.order, nullptr);
    EXPECT_EQ(result.order->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_FALSE(orderBook.doesOrderExist(2));
}

TEST_F(MatchingEngineSubmitNewTest, Submit_MarketOrder_SweepsLevelsAndNeverRests) {
    makeEngine();
    engine->submit(limit(1, 1, 100, 3, Side::Sell));
    engine->submit(limit(2, 2, 101, 4, Side::Sell));

    SubmitResult result = engine->submit(market(3, 3, 10, Side::Buy, 1002));

    EXPECT_EQ(result.reason, RejectionReason::None);
    ASSERT_NE(result.order, nullptr);
    EXPECT_EQ(result.order->getFilledQty(), static_cast<Quantity>(7));
    EXPECT_FALSE(orderBook.doesOrderExist(3));
    EXPECT_EQ(orderBook.getTradeExecutionCount(), 2u);
}

TEST_F(MatchingEngineSubmitNewTest, Submit_Trade_UpdatesLastTradedPrice) {
    makeEngine();
    engine->submit(limit(1, 1, 100, 5, Side::Sell));
    EXPECT_FALSE(engine->getLastTradedPrice().has_value());

    engine->submit(limit(2, 2, 101, 5, Side::Buy, 1001));

    ASSERT_TRUE(engine->getLastTradedPrice().has_value());
    EXPECT_EQ(*engine->getLastTradedPrice(), static_cast<PriceTicks>(100));
}

TEST_F(MatchingEngineSubmitNewTest, Submit_AcceptedOrder_CanBeCancelledThroughCancelRequest) {
    makeEngine();
    SubmitResult placed = engine->submit(limit(1, 1, 100, 10, Side::Buy));

    SubmitResult cancelled = engine->submit(CancelRequest(1, 1, 1001, 1));

    EXPECT_EQ(cancelled.reason, RejectionReason::None);
    EXPECT_EQ(cancelled.order, placed.order);
    EXPECT_EQ(placed.order->getStatus(), OrderStatus::Cancelled);
}

TEST_F(MatchingEngineSubmitNewTest, Submit_AcceptedOrder_CanBeModifiedThroughModifyRequest) {
    makeEngine();
    SubmitResult placed = engine->submit(limit(1, 1, 100, 10, Side::Buy));

    SubmitResult modified = engine->submit(ModifyRequest(1, 1, 1001, 1, std::nullopt, 6));

    EXPECT_EQ(modified.reason, RejectionReason::None);
    EXPECT_EQ(modified.order, placed.order);
    EXPECT_EQ(placed.order->getQty(), static_cast<Quantity>(6));
}
