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
    EXPECT_EQ(order->getOrderID(), 1);
    EXPECT_EQ(order->getOwnerID(), 1);
    EXPECT_EQ(order->getPriceTicks(), 0);
    EXPECT_EQ(order->getQty(), 10);
    EXPECT_EQ(order->getSide(), Side::Buy);
    EXPECT_EQ(order->getType(), OrderType::Limit);
    EXPECT_EQ(order->getTimestamp(), 1622547800);
    EXPECT_EQ(order->getStatus(), OrderStatus::Pending);
}

TEST_F(OrderTest, ReduceQtyDecreasesQuantity) {
    order->reduceQty(4);
    EXPECT_EQ(order->getQty(), 6);
    order->reduceQty(6);
    EXPECT_EQ(order->getQty(), 0);
}

TEST_F(OrderTest, SetStatusUpdatesOrderStatus) {
    order->setStatus(OrderStatus::PartiallyExecuted);
    EXPECT_EQ(order->getStatus(), OrderStatus::PartiallyExecuted);
    order->setStatus(OrderStatus::Executed);
    EXPECT_EQ(order->getStatus(), OrderStatus::Executed);
}

TEST_F(OrderTest, IsCancelledReturnsTrueForCancelledStatuses) {
    EXPECT_FALSE(order->isCancelled());
    order->setStatus(OrderStatus::Cancelled);
    EXPECT_TRUE(order->isCancelled());
    order->setStatus(OrderStatus::CancelledAfterPartialExecution);
    EXPECT_TRUE(order->isCancelled());
}

TEST_F(OrderTest, IsExecutedReturnsTrueForExecutedStatus) {
    EXPECT_FALSE(order->isExecuted());
    order->setStatus(OrderStatus::Executed);
    EXPECT_TRUE(order->isExecuted());
}