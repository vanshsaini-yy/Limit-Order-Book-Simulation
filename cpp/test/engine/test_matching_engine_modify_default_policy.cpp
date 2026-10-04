#include <gtest/gtest.h>
#include <memory>
#include "engine/matching_engine.hpp"
#include "models/modify_request.hpp"

class MatchingEngineModifyDefaultPolicyTest : public ::testing::Test {
protected:
    CancelBothSTP stpPolicy;
    DefaultModifyPolicy modifyPolicy;
    LimitOrderBook orderBook;
    std::optional<MatchingEngine> engine;

    void makeEngine(std::optional<PriceTicks> deviation = std::nullopt) {
        engine.emplace(&orderBook, &stpPolicy, nullptr, nullptr, deviation, &modifyPolicy);
    }

    void expectOrderState(OrderID orderId, PriceTicks price, Quantity qty, OrderStatus status) {
        OrderPtr order = orderBook.getOrder(orderId);
        ASSERT_NE(order, nullptr);
        EXPECT_EQ(order->getPriceTicks(), price);
        EXPECT_EQ(order->getQty(), qty);
        EXPECT_EQ(order->getStatus(), status);
    }
};

// =====================================================================
// Request validation
// =====================================================================

TEST_F(MatchingEngineModifyDefaultPolicyTest, Submit_ZeroTargetOrderID_IsRejected) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, 0, 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::InvalidModifyOrder);
    expectOrderState(resting->getOrderID(), 100, 10, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, Submit_NeitherPriceNorQtyGiven_IsRejectedAsNoOp) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), std::nullopt, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::NoOpModify);
    expectOrderState(resting->getOrderID(), 100, 10, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, Submit_ZeroNewPriceTicks_IsRejected) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), 0, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::InvalidModifyOrder);
    expectOrderState(resting->getOrderID(), 100, 10, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, Submit_NegativeNewPriceTicks_IsRejected) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), -5, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::InvalidModifyOrder);
    expectOrderState(resting->getOrderID(), 100, 10, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, Submit_ZeroNewOriginalQty_IsRejected) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), std::nullopt, 0);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::InvalidModifyOrder);
    expectOrderState(resting->getOrderID(), 100, 10, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, Submit_NegativeNewOriginalQty_IsRejected) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), std::nullopt, -3);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::InvalidModifyOrder);
    expectOrderState(resting->getOrderID(), 100, 10, OrderStatus::Pending);
}

// =====================================================================
// Order lookup
// =====================================================================

TEST_F(MatchingEngineModifyDefaultPolicyTest, Submit_NonExistentOrder_IsRejected) {
    makeEngine();
    OrderID nonExistentOrderID = 999;

    ModifyRequest request(1, 1, 1000, nonExistentOrderID, 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::OrderToBeModifiedDoesNotExist);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, Submit_AnotherOwnersOrder_IsRejected) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    OwnerID anotherOwnerID = 2;
    ModifyRequest request(1, anotherOwnerID, 1001, resting->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::OrderToBeModifiedDoesNotExist);
    expectOrderState(resting->getOrderID(), 100, 10, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, Submit_FullyExecutedOrder_IsRejected) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);
    OrderPtr fullFill = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(fullFill);

    ModifyRequest request(1, 1, 1002, resting->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::OrderToBeModifiedDoesNotExist);
    EXPECT_TRUE(resting->isExecuted());
}

// =====================================================================
// Quantity-only modify
// =====================================================================

TEST_F(MatchingEngineModifyDefaultPolicyTest, QuantityDecrease_DecreasesQuantity) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), std::nullopt, 6);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(resting->getQty(), static_cast<Quantity>(6));
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, QuantityIncrease_IncreasesQuantity) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), std::nullopt, 15);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    OrderPtr modifiedOrder = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(modifiedOrder, nullptr);
    EXPECT_EQ(modifiedOrder->getQty(), static_cast<Quantity>(15));
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, QuantityUnchanged_IsRejectedAsNoOp) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), std::nullopt, 10);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::NoOpModify);
    expectOrderState(resting->getOrderID(), 100, 10, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, QuantityDecrease_KeepsPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 1, 1002, firstResting->getOrderID(), std::nullopt, 6);
    engine->submit(request);

    OrderPtr incoming = std::make_shared<Order>(3, 3, 100, 6, Side::Sell, OrderType::Limit, 1003);
    engine->matchOrder(incoming);

    EXPECT_EQ(firstResting->getQty(), static_cast<Quantity>(0));
    EXPECT_TRUE(firstResting->isExecuted());
    EXPECT_EQ(secondResting->getQty(), static_cast<Quantity>(5));
    EXPECT_EQ(secondResting->getStatus(), OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, QuantityIncrease_LosesPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 1, 1002, firstResting->getOrderID(), std::nullopt, 12);
    engine->submit(request);
    
    OrderPtr incoming = std::make_shared<Order>(3, 3, 100, 5, Side::Sell, OrderType::Limit, 1003);
    engine->matchOrder(incoming);
    
    EXPECT_EQ(secondResting->getQty(), static_cast<Quantity>(0));
    EXPECT_TRUE(secondResting->isExecuted());
    OrderPtr modifiedOrder = orderBook.getOrder(firstResting->getOrderID());
    ASSERT_NE(modifiedOrder, nullptr);
    EXPECT_EQ(modifiedOrder->getQty(), static_cast<Quantity>(12));
    EXPECT_EQ(modifiedOrder->getStatus(), OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, QuantityUnchanged_KeepsPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 1, 1002, firstResting->getOrderID(), std::nullopt, 10);
    engine->submit(request);

    OrderPtr incoming = std::make_shared<Order>(3, 3, 100, 10, Side::Sell, OrderType::Limit, 1003);
    engine->matchOrder(incoming);

    EXPECT_EQ(firstResting->getQty(), static_cast<Quantity>(0));
    EXPECT_TRUE(firstResting->isExecuted());
    EXPECT_EQ(secondResting->getQty(), static_cast<Quantity>(5));
    EXPECT_EQ(secondResting->getStatus(), OrderStatus::Pending);
}

// =====================================================================
// Price-only modify
// =====================================================================

TEST_F(MatchingEngineModifyDefaultPolicyTest, PriceDecrease_DecreasesPrice) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), 95, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    OrderPtr modifiedOrder = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(modifiedOrder, nullptr);
    EXPECT_EQ(modifiedOrder->getPriceTicks(), static_cast<PriceTicks>(95));
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, PriceIncrease_IncreasesPrice) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    OrderPtr modifiedOrder = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(modifiedOrder, nullptr);
    EXPECT_EQ(modifiedOrder->getPriceTicks(), static_cast<PriceTicks>(105));
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, PriceUnchanged_IsRejectedAsNoOp) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), 100, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::NoOpModify);
    expectOrderState(resting->getOrderID(), 100, 10, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, PriceIncrease_LosesPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 105, 5, Side::Buy, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 2, 1002, secondResting->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);
    EXPECT_EQ(result, RejectionReason::None);

    OrderPtr incoming = std::make_shared<Order>(3, 3, 105, 5, Side::Sell, OrderType::Limit, 1003);
    engine->matchOrder(incoming);

    EXPECT_EQ(firstResting->getQty(), static_cast<Quantity>(0));
    EXPECT_TRUE(firstResting->isExecuted());
    expectOrderState(secondResting->getOrderID(), 105, 10, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, PriceDecrease_LosesPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 95, 5, Side::Buy, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 2, 1002, secondResting->getOrderID(), 95, std::nullopt);
    RejectionReason result = engine->submit(request);
    EXPECT_EQ(result, RejectionReason::None);

    OrderPtr incoming = std::make_shared<Order>(3, 3, 95, 5, Side::Sell, OrderType::Limit, 1003);
    engine->matchOrder(incoming);

    EXPECT_EQ(firstResting->getQty(), static_cast<Quantity>(0));
    EXPECT_TRUE(firstResting->isExecuted());
    expectOrderState(secondResting->getOrderID(), 95, 10, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, PriceUnchanged_KeepsPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 1, 1002, firstResting->getOrderID(), 100, 6);
    RejectionReason result = engine->submit(request);
    EXPECT_EQ(result, RejectionReason::None);

    OrderPtr incoming = std::make_shared<Order>(3, 3, 100, 6, Side::Sell, OrderType::Limit, 1003);
    engine->matchOrder(incoming);

    EXPECT_EQ(firstResting->getQty(), static_cast<Quantity>(0));
    EXPECT_TRUE(firstResting->isExecuted());
    EXPECT_EQ(secondResting->getQty(), static_cast<Quantity>(5));
    EXPECT_EQ(secondResting->getStatus(), OrderStatus::Pending);
}

// =====================================================================
// Combined price and quantity modify
// =====================================================================

TEST_F(MatchingEngineModifyDefaultPolicyTest, PriceAndQuantityChange_BothReflectedInOrder) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), 105, 6);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    OrderPtr modifiedOrder = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(modifiedOrder, nullptr);
    EXPECT_EQ(modifiedOrder->getPriceTicks(), static_cast<PriceTicks>(105));
    EXPECT_EQ(modifiedOrder->getQty(), static_cast<Quantity>(6));
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, PriceAndQuantityUnchanged_IsRejectedAsNoOp) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), 100, 10);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::NoOpModify);
    expectOrderState(resting->getOrderID(), 100, 10, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, PriceChangeWithQuantityIncrease_LosesPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 105, 5, Side::Buy, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 2, 1002, secondResting->getOrderID(), 105, 15);
    RejectionReason result = engine->submit(request);
    EXPECT_EQ(result, RejectionReason::None);

    OrderPtr incoming = std::make_shared<Order>(3, 3, 105, 5, Side::Sell, OrderType::Limit, 1003);
    engine->matchOrder(incoming);

    EXPECT_EQ(firstResting->getQty(), static_cast<Quantity>(0));
    EXPECT_TRUE(firstResting->isExecuted());
    expectOrderState(secondResting->getOrderID(), 105, 15, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, PriceChangeWithQuantityDecrease_LosesPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 105, 5, Side::Buy, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 2, 1002, secondResting->getOrderID(), 105, 6);
    RejectionReason result = engine->submit(request);
    EXPECT_EQ(result, RejectionReason::None);

    OrderPtr incoming = std::make_shared<Order>(3, 3, 105, 5, Side::Sell, OrderType::Limit, 1003);
    engine->matchOrder(incoming);

    EXPECT_EQ(firstResting->getQty(), static_cast<Quantity>(0));
    EXPECT_TRUE(firstResting->isExecuted());
    expectOrderState(secondResting->getOrderID(), 105, 6, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, PriceAndQuantityUnchanged_KeepsPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 1, 1002, firstResting->getOrderID(), 100, 10);
    RejectionReason result = engine->submit(request);
    EXPECT_EQ(result, RejectionReason::NoOpModify);

    OrderPtr incoming = std::make_shared<Order>(3, 3, 100, 10, Side::Sell, OrderType::Limit, 1003);
    engine->matchOrder(incoming);

    EXPECT_EQ(firstResting->getQty(), static_cast<Quantity>(0));
    EXPECT_TRUE(firstResting->isExecuted());
    EXPECT_EQ(secondResting->getQty(), static_cast<Quantity>(5));
    EXPECT_EQ(secondResting->getStatus(), OrderStatus::Pending);
}

// =====================================================================
// Partially filled orders
// =====================================================================

TEST_F(MatchingEngineModifyDefaultPolicyTest, PartiallyFilled_NewTotalBelowFilled_IsRejected) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);
    OrderPtr partialFill = std::make_shared<Order>(2, 2, 100, 4, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(partialFill);

    ModifyRequest request(1, 1, 1002, resting->getOrderID(), std::nullopt, 3);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::ModifyQuantityBelowFilled);
    expectOrderState(resting->getOrderID(), 100, 6, OrderStatus::PartiallyExecuted);
    EXPECT_EQ(resting->getOriginalQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, PartiallyFilled_NewTotalEqualsFilled_CancelsRemainderAsDoneForDay) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);
    OrderPtr partialFill = std::make_shared<Order>(2, 2, 100, 4, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(partialFill);

    ModifyRequest request(1, 1, 1002, resting->getOrderID(), std::nullopt, 4);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_FALSE(orderBook.doesOrderExist(resting->getOrderID()));
    EXPECT_EQ(resting->getStatus(), OrderStatus::CancelledAfterPartialExecution);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, PartiallyFilled_NewTotalEqualsFilled_RecordsCancellation) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);
    OrderPtr partialFill = std::make_shared<Order>(2, 2, 100, 4, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(partialFill);

    ModifyRequest request(1, 1, 1002, resting->getOrderID(), std::nullopt, 4);
    engine->submit(request);

    EXPECT_EQ(orderBook.getOrderCancellationCount(), 1u);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, PartiallyFilled_NewTotalEqualsFilledWithPriceChange_IsDoneForDay) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);
    OrderPtr partialFill = std::make_shared<Order>(2, 2, 100, 4, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(partialFill);

    ModifyRequest request(1, 1, 1002, resting->getOrderID(), 105, 4);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_FALSE(orderBook.doesOrderExist(resting->getOrderID()));
    EXPECT_EQ(resting->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(orderBook.getOrderCancellationCount(), 1u);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, PartiallyFilled_QuantityDecrease_KeepsPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);
    OrderPtr partialFill = std::make_shared<Order>(3, 3, 100, 4, Side::Sell, OrderType::Limit, 1002);
    engine->matchOrder(partialFill);

    ModifyRequest request(1, 1, 1003, firstResting->getOrderID(), std::nullopt, 8);
    RejectionReason result = engine->submit(request);
    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(firstResting->getQty(), static_cast<Quantity>(4));

    OrderPtr incoming = std::make_shared<Order>(4, 4, 100, 4, Side::Sell, OrderType::Limit, 1004);
    engine->matchOrder(incoming);

    EXPECT_EQ(firstResting->getQty(), static_cast<Quantity>(0));
    EXPECT_TRUE(firstResting->isExecuted());
    EXPECT_EQ(secondResting->getQty(), static_cast<Quantity>(5));
    EXPECT_EQ(secondResting->getStatus(), OrderStatus::Pending);
}

// =====================================================================
// Replacement order construction
// =====================================================================

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_PendingOrder_RestsWithCorrectQuantity) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), 105, 12);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    OrderPtr replacement = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(replacement, nullptr);
    EXPECT_EQ(replacement->getQty(), static_cast<Quantity>(12));
    EXPECT_EQ(replacement->getOriginalQty(), static_cast<Quantity>(12));
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_PartiallyFilledOrder_RestsWithCorrectQuantity) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);
    OrderPtr partialFill = std::make_shared<Order>(2, 2, 100, 4, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(partialFill);

    ModifyRequest request(1, 1, 1002, resting->getOrderID(), 105, 12);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    OrderPtr replacement = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(replacement, nullptr);
    EXPECT_EQ(replacement->getQty(), static_cast<Quantity>(8));
    EXPECT_EQ(replacement->getOriginalQty(), static_cast<Quantity>(12));
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_PendingOrder_RemainsPending) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    OrderPtr replacement = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(replacement, nullptr);
    EXPECT_EQ(replacement->getStatus(), OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_PartiallyFilledOrder_RemainsPartiallyFilled) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);
    OrderPtr partialFill = std::make_shared<Order>(2, 2, 100, 4, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(partialFill);

    ModifyRequest request(1, 1, 1002, resting->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    OrderPtr replacement = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(replacement, nullptr);
    EXPECT_EQ(replacement->getStatus(), OrderStatus::PartiallyExecuted);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_ReplacementUsesRequestTimestamp) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    Timestamp requestTimestamp = 1001;
    ModifyRequest request(1, 1, requestTimestamp, resting->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    OrderPtr replacement = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(replacement, nullptr);
    EXPECT_EQ(replacement->getTimestamp(), requestTimestamp);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_ReplacementKeepsOrderIDAndOwner) {
    makeEngine();
    OrderID originalOrderID = 1;
    OwnerID originalOwnerID = 7;
    OrderPtr resting = std::make_shared<Order>(originalOrderID, originalOwnerID, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, originalOwnerID, 1001, originalOrderID, 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    OrderPtr replacement = orderBook.getOrder(originalOrderID);
    ASSERT_NE(replacement, nullptr);
    EXPECT_EQ(replacement->getOrderID(), originalOrderID);
    EXPECT_EQ(replacement->getOwnerID(), originalOwnerID);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_ReplacementCarriesOverSide) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    OrderPtr replacement = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(replacement, nullptr);
    EXPECT_EQ(replacement->getSide(), Side::Sell);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_ReplacementCarriesOverType) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    OrderPtr replacement = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(replacement, nullptr);
    EXPECT_EQ(replacement->getType(), OrderType::Limit);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_ReplacementCarriesOverTimeInForce) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000, TimeInForce::GTC);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    OrderPtr replacement = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(replacement, nullptr);
    EXPECT_EQ(replacement->getTimeInForce(), TimeInForce::GTC);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_ReplacementCarriesOverPostOnlyFlag) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000, TimeInForce::GTC, true);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), 95, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    OrderPtr replacement = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(replacement, nullptr);
    EXPECT_TRUE(replacement->isPostOnly());
}

// =====================================================================
// Cancel-replace mechanics
// =====================================================================

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_OriginalOrderHandleIsCancelled) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_TRUE(resting->isCancelled());
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModifyTwice_BothApply) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest firstRequest(1, 1, 1001, resting->getOrderID(), 105, std::nullopt);
    RejectionReason firstResult = engine->submit(firstRequest);
    EXPECT_EQ(firstResult, RejectionReason::None);

    ModifyRequest secondRequest(2, 1, 1002, resting->getOrderID(), 110, std::nullopt);
    RejectionReason secondResult = engine->submit(secondRequest);
    EXPECT_EQ(secondResult, RejectionReason::None);

    OrderPtr replacement = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(replacement, nullptr);
    EXPECT_EQ(replacement->getPriceTicks(), static_cast<PriceTicks>(110));
    EXPECT_EQ(replacement->getQty(), static_cast<Quantity>(10));
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_DoesNotRecordCancellation) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(orderBook.getOrderCancellationCount(), 0u);
}

// =====================================================================
// Cancel-replace rejections
// =====================================================================

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_RejectedByPriceCollar_OrderStaysResting) {
    makeEngine(5);
    OrderPtr restingBuy = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr otherRestingBuy = std::make_shared<Order>(2, 2, 95, 3, Side::Buy, OrderType::Limit, 1001);
    OrderPtr restingSell = std::make_shared<Order>(3, 3, 110, 5, Side::Sell, OrderType::Limit, 1002);
    engine->matchOrder(restingBuy);
    engine->matchOrder(otherRestingBuy);
    engine->matchOrder(restingSell);

    ModifyRequest request(1, 1, 1003, restingBuy->getOrderID(), 200, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    expectOrderState(restingBuy->getOrderID(), 100, 10, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_RejectedByPriceCollar_KeepsQueuePriority) {
    makeEngine(5);
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    OrderPtr restingSell = std::make_shared<Order>(3, 3, 110, 5, Side::Sell, OrderType::Limit, 1002);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);
    engine->matchOrder(restingSell);

    ModifyRequest request(1, 1, 1003, firstResting->getOrderID(), 200, std::nullopt);
    RejectionReason result = engine->submit(request);
    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);

    OrderPtr incoming = std::make_shared<Order>(4, 4, 100, 10, Side::Sell, OrderType::Limit, 1004);
    engine->matchOrder(incoming);

    EXPECT_EQ(firstResting->getQty(), static_cast<Quantity>(0));
    EXPECT_EQ(firstResting->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(secondResting->getQty(), static_cast<Quantity>(5));
    EXPECT_EQ(secondResting->getStatus(), OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_NoLastTrade_MidReferenceIncludesOrderBeingModified) {
    makeEngine(5);
    OrderPtr restingBuy = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr restingSell = std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(restingBuy);
    engine->matchOrder(restingSell);

    ModifyRequest request(1, 1, 1002, restingBuy->getOrderID(), 200, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    expectOrderState(restingBuy->getOrderID(), 100, 10, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_RejectedByPostOnly_OrderStaysResting) {
    makeEngine();
    OrderPtr restingBuy = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000, TimeInForce::GTC, true);
    OrderPtr restingSell = std::make_shared<Order>(2, 2, 105, 5, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(restingBuy);
    engine->matchOrder(restingSell);

    ModifyRequest request(1, 1, 1002, restingBuy->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::PostOnlyWouldCross);
    expectOrderState(restingBuy->getOrderID(), 100, 10, OrderStatus::Pending);
}

// =====================================================================
// Matching on a repriced modify
// =====================================================================

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_CrossingBook_ExecutesAgainstRestingOrder) {
    makeEngine();
    OrderPtr restingBuy = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr restingSell = std::make_shared<Order>(2, 2, 105, 4, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(restingBuy);
    engine->matchOrder(restingSell);

    ModifyRequest request(1, 1, 1002, restingBuy->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(restingSell->getQty(), static_cast<Quantity>(0));
    EXPECT_EQ(restingSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(orderBook.getTradeExecutionCount(), 1u);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_CrossingBook_RemainderRestsAsPartiallyExecuted) {
    makeEngine();
    OrderPtr restingBuy = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr restingSell = std::make_shared<Order>(2, 2, 105, 4, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(restingBuy);
    engine->matchOrder(restingSell);

    ModifyRequest request(1, 1, 1002, restingBuy->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    expectOrderState(restingBuy->getOrderID(), 105, 6, OrderStatus::PartiallyExecuted);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_CrossingBook_FullyFilledReplacementLeavesBook) {
    makeEngine();
    OrderPtr restingBuy = std::make_shared<Order>(1, 1, 100, 4, Side::Buy, OrderType::Limit, 1000);
    OrderPtr restingSell = std::make_shared<Order>(2, 2, 105, 10, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(restingBuy);
    engine->matchOrder(restingSell);

    ModifyRequest request(1, 1, 1002, restingBuy->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_FALSE(orderBook.doesOrderExist(restingBuy->getOrderID()));
    EXPECT_EQ(restingSell->getQty(), static_cast<Quantity>(6));
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_CrossingBook_UpdatesLastTradedPrice) {
    makeEngine();
    OrderPtr restingBuy = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr restingSell = std::make_shared<Order>(2, 2, 105, 4, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(restingBuy);
    engine->matchOrder(restingSell);
    EXPECT_FALSE(engine->getLastTradedPrice().has_value());

    ModifyRequest request(1, 1, 1002, restingBuy->getOrderID(), 110, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::None);
    ASSERT_TRUE(engine->getLastTradedPrice().has_value());
    EXPECT_EQ(*engine->getLastTradedPrice(), static_cast<PriceTicks>(105));
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, RepricedModify_CrossingOwnOrder_IsRejectedBySelfTradePrevention) {
    makeEngine();
    OrderPtr restingBuy = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr ownRestingSell = std::make_shared<Order>(2, 1, 105, 4, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(restingBuy);
    engine->matchOrder(ownRestingSell);

    ModifyRequest request(1, 1, 1002, restingBuy->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);

    EXPECT_EQ(result, RejectionReason::SelfTradePrevention);
    EXPECT_FALSE(orderBook.doesOrderExist(restingBuy->getOrderID()));
    EXPECT_FALSE(orderBook.doesOrderExist(ownRestingSell->getOrderID()));
}

// =====================================================================
// Sell side
// =====================================================================

TEST_F(MatchingEngineModifyDefaultPolicyTest, Sell_QuantityDecrease_KeepsPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 5, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 1, 1002, firstResting->getOrderID(), std::nullopt, 6);
    RejectionReason result = engine->submit(request);
    EXPECT_EQ(result, RejectionReason::None);

    OrderPtr incoming = std::make_shared<Order>(3, 3, 100, 6, Side::Buy, OrderType::Limit, 1003);
    engine->matchOrder(incoming);

    EXPECT_TRUE(firstResting->isExecuted());
    expectOrderState(secondResting->getOrderID(), 100, 5, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, Sell_QuantityIncrease_LosesPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 5, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 1, 1002, firstResting->getOrderID(), std::nullopt, 12);
    RejectionReason result = engine->submit(request);
    EXPECT_EQ(result, RejectionReason::None);

    OrderPtr incoming = std::make_shared<Order>(3, 3, 100, 5, Side::Buy, OrderType::Limit, 1003);
    engine->matchOrder(incoming);

    EXPECT_TRUE(secondResting->isExecuted());
    expectOrderState(firstResting->getOrderID(), 100, 12, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyDefaultPolicyTest, Sell_PriceIncrease_LosesPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 105, 5, Side::Sell, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 1, 1002, firstResting->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request);
    EXPECT_EQ(result, RejectionReason::None);

    OrderPtr incoming = std::make_shared<Order>(3, 3, 105, 5, Side::Buy, OrderType::Limit, 1003);
    engine->matchOrder(incoming);

    EXPECT_TRUE(secondResting->isExecuted());
    expectOrderState(firstResting->getOrderID(), 105, 10, OrderStatus::Pending);
}

// =====================================================================
// Policy injection
// =====================================================================

TEST_F(MatchingEngineModifyDefaultPolicyTest, Submit_NoModifyPolicyInjected_UsesDefaultPolicy) {
    engine.emplace(&orderBook, &stpPolicy);
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 1, 1002, firstResting->getOrderID(), std::nullopt, 12);
    RejectionReason result = engine->submit(request);
    EXPECT_EQ(result, RejectionReason::None);

    OrderPtr incoming = std::make_shared<Order>(3, 3, 100, 5, Side::Sell, OrderType::Limit, 1003);
    engine->matchOrder(incoming);

    EXPECT_TRUE(secondResting->isExecuted());
    expectOrderState(firstResting->getOrderID(), 100, 12, OrderStatus::Pending);
}
