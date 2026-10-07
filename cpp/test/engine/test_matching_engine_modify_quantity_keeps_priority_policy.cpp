#include <gtest/gtest.h>
#include <memory>
#include "engine/matching_engine.hpp"
#include "models/modify_request.hpp"

class MatchingEngineModifyQuantityKeepsPriorityPolicyTest : public ::testing::Test {
protected:
    CancelBothSTP stpPolicy;
    QuantityKeepsPriorityModifyPolicy modifyPolicy;
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

TEST_F(MatchingEngineModifyQuantityKeepsPriorityPolicyTest, QuantityIncrease_IncreasesQuantityInPlace) {
    makeEngine();
    OrderPtr resting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    engine->matchOrder(resting);

    ModifyRequest request(1, 1, 1001, resting->getOrderID(), std::nullopt, 15);
    RejectionReason result = engine->submit(request).reason;

    EXPECT_EQ(result, RejectionReason::None);
    expectOrderState(resting->getOrderID(), 100, 15, OrderStatus::Pending);
    OrderPtr modifiedOrder = orderBook.getOrder(resting->getOrderID());
    ASSERT_NE(modifiedOrder, nullptr);
    EXPECT_EQ(modifiedOrder->getTimestamp(), static_cast<Timestamp>(1000));
}

TEST_F(MatchingEngineModifyQuantityKeepsPriorityPolicyTest, QuantityIncrease_KeepsPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 1, 1002, firstResting->getOrderID(), std::nullopt, 12);
    RejectionReason result = engine->submit(request).reason;
    EXPECT_EQ(result, RejectionReason::None);

    OrderPtr incoming = std::make_shared<Order>(3, 3, 100, 12, Side::Sell, OrderType::Limit, 1003);
    engine->matchOrder(incoming);

    EXPECT_TRUE(firstResting->isExecuted());
    expectOrderState(secondResting->getOrderID(), 100, 5, OrderStatus::Pending);
}

TEST_F(MatchingEngineModifyQuantityKeepsPriorityPolicyTest, PriceChange_LosesPriority) {
    makeEngine();
    OrderPtr firstResting = std::make_shared<Order>(1, 1, 105, 5, Side::Buy, OrderType::Limit, 1000);
    OrderPtr secondResting = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    engine->matchOrder(firstResting);
    engine->matchOrder(secondResting);

    ModifyRequest request(1, 2, 1002, secondResting->getOrderID(), 105, std::nullopt);
    RejectionReason result = engine->submit(request).reason;
    EXPECT_EQ(result, RejectionReason::None);

    OrderPtr incoming = std::make_shared<Order>(3, 3, 105, 5, Side::Sell, OrderType::Limit, 1003);
    engine->matchOrder(incoming);

    EXPECT_TRUE(firstResting->isExecuted());
    expectOrderState(secondResting->getOrderID(), 105, 10, OrderStatus::Pending);
}
