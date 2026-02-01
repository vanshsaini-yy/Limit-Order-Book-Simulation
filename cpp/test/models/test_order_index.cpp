#include <gtest/gtest.h>
#include <memory>
#include <list>
#include "models/order_index.hpp"
#include "models/order.hpp"

TEST(OrderIndexTest, GettersAndIterator) {
    std::list<std::shared_ptr<Order>> orders;
    auto o = std::make_shared<Order>(10ull, 100ull, 5u, Side::Buy, OrderType::Limit, 111u);
    orders.push_back(o);
    auto it = std::prev(orders.end());

    OrderIndex idx(1, 100ull, it);

    EXPECT_EQ(idx.getIsBuy(), 1);
    EXPECT_EQ(idx.getPriceTicks(), 100ull);

    auto iter = idx.getOrderIter();
    EXPECT_EQ(*iter, o);
    EXPECT_EQ((*iter)->getId(), 10ull);
    EXPECT_EQ((*iter)->getPriceTicks(), 100ull);
    EXPECT_EQ((*iter)->getQty(), 5u);
}

TEST(OrderIndexTest, MultipleOrdersIterator) {
    std::list<std::shared_ptr<Order>> orders;
    auto a = std::make_shared<Order>(1ull, 50ull, 2u, Side::Sell, OrderType::Limit, 10u);
    auto b = std::make_shared<Order>(2ull, 50ull, 3u, Side::Sell, OrderType::Limit, 11u);
    orders.push_back(a);
    orders.push_back(b);

    auto it_b = std::prev(orders.end());
    OrderIndex idx_b(0, 50ull, it_b);

    auto iter = idx_b.getOrderIter();
    EXPECT_EQ(*iter, b);
    EXPECT_EQ((*iter)->getId(), 2ull);
}
