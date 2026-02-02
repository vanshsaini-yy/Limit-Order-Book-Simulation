#include <gtest/gtest.h>
#include "models/order.hpp"

class OrderTest : public ::testing::Test {
protected:
    Order* order;

    void SetUp() override {
        order = new Order(1, 1, 0, 10, Side::Buy, OrderType::Limit, 1622547800);
    }

    void TearDown() override {
        delete order;
    }
};

TEST_F(OrderTest, GettersReturnExpectedValues) {
    EXPECT_EQ(order->getOrderID(), 1u);
    EXPECT_EQ(order->getOwnerID(), 1u);
    EXPECT_EQ(order->getPriceTicks(), 0ull);
    EXPECT_EQ(order->getQty(), 10u);
    EXPECT_EQ(order->getSide(), Side::Buy);
    EXPECT_EQ(order->getType(), OrderType::Limit);
    EXPECT_EQ(order->getTimestamp(), 1622547800ull);
}

TEST_F(OrderTest, DefaultBehaviorIsConstexprAccessible) {
    constexpr Order o2(2, 2, 5, 3, Side::Sell, OrderType::Market, 1234567890);
    EXPECT_EQ(o2.getOrderID(), 2u);
    EXPECT_EQ(o2.getOwnerID(), 2u);
    EXPECT_EQ(o2.getPriceTicks(), 5ull);
    EXPECT_EQ(o2.getQty(), 3u);
    EXPECT_EQ(o2.getSide(), Side::Sell);
    EXPECT_EQ(o2.getType(), OrderType::Market);
    EXPECT_EQ(o2.getTimestamp(), 1234567890ull);
}