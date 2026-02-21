#include <gtest/gtest.h>
#include "models/order.hpp"

class OrderTest : public ::testing::Test {
protected:
    Order* order1;
    Order* order2;

    void SetUp() override {
        order1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
        order2 = new Order(2, 1, 0, 0, Side::None, OrderType::Cancel, 1622547801, 1);
    }

    void TearDown() override {
        delete order1;
        delete order2;
    }
};

TEST_F(OrderTest, GettersReturnExpectedValues) {
    EXPECT_EQ(order1->getOrderID(), 1);
    EXPECT_EQ(order1->getOwnerID(), 1);
    EXPECT_EQ(order1->getPriceTicks(), 100);
    EXPECT_EQ(order1->getQty(), 10);
    EXPECT_EQ(order1->getSide(), Side::Buy);
    EXPECT_EQ(order1->getType(), OrderType::Limit);
    EXPECT_EQ(order1->getTimestamp(), 1622547800);
    EXPECT_EQ(order1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(order1->getLinkedOrderID(), 0);

    EXPECT_EQ(order2->getOrderID(), 2);
    EXPECT_EQ(order2->getOwnerID(), 1);
    EXPECT_EQ(order2->getPriceTicks(), 0);
    EXPECT_EQ(order2->getQty(), 0);
    EXPECT_EQ(order2->getSide(), Side::None);
    EXPECT_EQ(order2->getType(), OrderType::Cancel);
    EXPECT_EQ(order2->getTimestamp(), 1622547801);
    EXPECT_EQ(order2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(order2->getLinkedOrderID(), 1);
}

TEST_F(OrderTest, ReduceQtyDecreasesQuantity) {
    order1->reduceQty(4);
    EXPECT_EQ(order1->getQty(), 6);
    order1->reduceQty(6);
    EXPECT_EQ(order1->getQty(), 0);
}

TEST_F(OrderTest, SetStatusUpdatesOrderStatus) {
    order1->setStatus(OrderStatus::PartiallyExecuted);
    EXPECT_EQ(order1->getStatus(), OrderStatus::PartiallyExecuted);
    order1->setStatus(OrderStatus::Executed);
    EXPECT_EQ(order1->getStatus(), OrderStatus::Executed);
}

TEST_F(OrderTest, IsCancelledReturnsFalseForNonCancelledStatuses) {
    order1->setStatus(OrderStatus::Pending);
    EXPECT_FALSE(order1->isCancelled());
    order1->setStatus(OrderStatus::PartiallyExecuted);
    EXPECT_FALSE(order1->isCancelled());
    order1->setStatus(OrderStatus::Executed);
    EXPECT_FALSE(order1->isCancelled());
}

TEST_F(OrderTest, IsCancelledReturnsTrueForCancelledStatuses) {
    order1->setStatus(OrderStatus::Cancelled);
    EXPECT_TRUE(order1->isCancelled());
    order1->setStatus(OrderStatus::CancelledAfterPartialExecution);
    EXPECT_TRUE(order1->isCancelled());
}

TEST_F(OrderTest, IsExecutedReturnsFalseForNonExecutedStatuses) {
    order1->setStatus(OrderStatus::Pending);
    EXPECT_FALSE(order1->isExecuted());
    order1->setStatus(OrderStatus::PartiallyExecuted);
    EXPECT_FALSE(order1->isExecuted());
    order1->setStatus(OrderStatus::Cancelled);
    EXPECT_FALSE(order1->isExecuted());
    order1->setStatus(OrderStatus::CancelledAfterPartialExecution);
    EXPECT_FALSE(order1->isExecuted());
}

TEST_F(OrderTest, IsExecutedReturnsTrueForExecutedStatus) {
    order1->setStatus(OrderStatus::Executed);
    EXPECT_TRUE(order1->isExecuted());
}