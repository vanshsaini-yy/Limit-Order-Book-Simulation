#include <gtest/gtest.h>
#include <memory>
#include <list>
#include "models/order_index.hpp"
#include "models/order.hpp"

class OrderIndexTest : public ::testing::Test {
};

TEST_F(OrderIndexTest, ConstructorAndGetters) {
    std::list<OrderPtr> orders;
    auto order1 = new Order(421, 1, 10, 100, Side::Buy, OrderType::Limit, 1001);
    auto order2 = new Order(422, 1, 20, 200, Side::Sell, OrderType::Limit, 1002);
    orders.push_back(order1);
    orders.push_back(order2);
    auto it2 = std::prev(orders.end());
    auto it1 = std::prev(it2);

    OrderIndex index1(1, order1->getPriceTicks(), it1);
    OrderIndex index2(0, order2->getPriceTicks(), it2);

    EXPECT_EQ(index1.getIsBuy(), 1);
    EXPECT_EQ(index1.getPriceTicks(), 10ull);
    EXPECT_EQ(index1.getOrderIter(), it1);
    EXPECT_EQ(index2.getIsBuy(), 0);
    EXPECT_EQ(index2.getPriceTicks(), 20ull);
    EXPECT_EQ(index2.getOrderIter(), it2);
}
