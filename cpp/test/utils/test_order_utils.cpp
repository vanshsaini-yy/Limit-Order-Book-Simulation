#include<gtest/gtest.h>
#include "utils/order_utils.hpp"

TEST(IsSelfTrade, TestSelfTradeDetection) {
    auto order1 = std::make_unique<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    auto order2 = std::make_unique<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1001);
    auto order3 = std::make_unique<Order>(3, 2, 100, 10, Side::Sell, OrderType::Limit, 1002);

    EXPECT_TRUE(isSelfTrade(order1.get(), order2.get()));
    EXPECT_FALSE(isSelfTrade(order1.get(), order3.get()));
}